#include "lsp/lsp_client.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

namespace xenon::lsp {

LspClient::LspClient() = default;

LspClient::~LspClient() {
    stop();
}

bool LspClient::start(const std::vector<std::string>& command,
                      const std::string& rootUri) {
    if (command.empty()) return false;

    // Create two pipes: parent writes to server stdin, reads from server stdout
    int stdin_pipe[2];   // parent writes [1], server reads [0]
    int stdout_pipe[2];  // server writes [1], parent reads [0]

    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        return false;
    }

    if (pid == 0) {
        // Child process: become the language server
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);

        // Build argv
        std::vector<char*> argv;
        std::vector<std::string> args = command;
        for (auto& a : args) argv.push_back(a.data());
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        _exit(127); // exec failed
    }

    // Parent: close child-end of pipes
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    server_stdin_fd_  = stdin_pipe[1];
    server_stdout_fd_ = stdout_pipe[0];
    server_pid_ = pid;
    running_ = true;

    // Start reader thread
    reader_thread_ = std::thread(&LspClient::readerThread, this);

    // Send initialize
    JsonObject initParams;
    initParams["processId"] = static_cast<int64_t>(getpid());
    initParams["rootUri"] = rootUri;

    JsonObject caps;
    JsonObject textDocCaps;
    JsonObject syncCaps;
    syncCaps["dynamicRegistration"] = false;
    syncCaps["willSave"] = false;
    syncCaps["didSave"] = true;
    syncCaps["openClose"] = true;
    textDocCaps["synchronization"] = JsonValue(std::move(syncCaps));

    JsonObject completionCaps;
    JsonObject completionItemCaps;
    completionItemCaps["snippetSupport"] = false;
    completionCaps["completionItem"] = JsonValue(std::move(completionItemCaps));
    textDocCaps["completion"] = JsonValue(std::move(completionCaps));

    textDocCaps["hover"] = JsonValue(JsonObject{});
    textDocCaps["definition"] = JsonValue(JsonObject{});
    textDocCaps["publishDiagnostics"] = JsonValue(JsonObject{});
    caps["textDocument"] = JsonValue(std::move(textDocCaps));
    initParams["capabilities"] = JsonValue(std::move(caps));

    int id = next_id_++;
    sendMessage(buildRequest(id, "initialize", JsonValue(std::move(initParams))));
    (void)rootUri;

    return true;
}

void LspClient::stop() {
    if (!running_) return;
    running_ = false;

    // Send shutdown + exit
    try {
        int id = next_id_++;
        sendMessage(buildRequest(id, "shutdown", JsonValue()));
        sendMessage(buildNotification("exit", JsonValue()));
    } catch (...) {}

    if (server_stdin_fd_ >= 0)  { close(server_stdin_fd_);  server_stdin_fd_ = -1; }
    if (server_stdout_fd_ >= 0) { close(server_stdout_fd_); server_stdout_fd_ = -1; }

    if (reader_thread_.joinable()) reader_thread_.join();

    if (server_pid_ > 0) {
        waitpid(server_pid_, nullptr, WNOHANG);
        server_pid_ = -1;
    }
}

void LspClient::sendMessage(const std::string& json) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    if (server_stdin_fd_ < 0) return;
    std::string encoded = lspEncode(json);
    const char* ptr = encoded.data();
    size_t remaining = encoded.size();
    while (remaining > 0) {
        ssize_t written = write(server_stdin_fd_, ptr, remaining);
        if (written <= 0) break;
        ptr += written;
        remaining -= static_cast<size_t>(written);
    }
}

void LspClient::readerThread() {
    char buf[4096];
    while (running_ && server_stdout_fd_ >= 0) {
        ssize_t n = read(server_stdout_fd_, buf, sizeof(buf));
        if (n <= 0) break;
        read_buf_.append(buf, static_cast<size_t>(n));

        // Process complete messages
        while (true) {
            // Find Content-Length header
            auto header_end = read_buf_.find("\r\n\r\n");
            if (header_end == std::string::npos) break;

            std::string header = read_buf_.substr(0, header_end);
            size_t content_length = 0;
            const std::string cl_prefix = "Content-Length: ";
            auto cl_pos = header.find(cl_prefix);
            if (cl_pos != std::string::npos) {
                content_length = std::stoull(
                    header.substr(cl_pos + cl_prefix.size()));
            }

            size_t body_start = header_end + 4;
            if (read_buf_.size() < body_start + content_length) break;

            std::string body = read_buf_.substr(body_start, content_length);
            read_buf_.erase(0, body_start + content_length);

            try {
                JsonValue msg = jsonParse(body);
                processMessage(msg);
            } catch (...) {
                // Ignore malformed messages
            }
        }
    }
    // Server process exited or pipe was closed — mark as not running so
    // callers see the correct state via isRunning().
    running_ = false;
    initialized_ = false;
}

void LspClient::processMessage(const JsonValue& msg) {
    if (!msg.isObject()) return;

    if (msg.has("method")) {
        std::string method = msg["method"].asString();
        const JsonValue& params = msg["params"];
        processNotification(method, params);
    } else if (msg.has("id") && msg["id"].isInt()) {
        int id = static_cast<int>(msg["id"].asInt());
        processResponse(id, msg["result"], msg["error"]);
    }
}

void LspClient::processNotification(const std::string& method,
                                    const JsonValue& params) {
    if (method == "textDocument/publishDiagnostics") {
        if (!params.isObject()) return;
        std::string uri = params["uri"].isString() ? params["uri"].asString() : "";
        std::vector<Diagnostic> diags;

        if (params.has("diagnostics") && params["diagnostics"].isArray()) {
            for (const auto& d : params["diagnostics"].asArray()) {
                if (!d.isObject()) continue;
                Diagnostic diag;
                if (d.has("range") && d["range"].isObject()) {
                    const auto& range = d["range"];
                    if (range.has("start") && range["start"].isObject()) {
                        diag.line = static_cast<int>(range["start"]["line"].isInt() ?
                            range["start"]["line"].asInt() : 0);
                        diag.col = static_cast<int>(range["start"]["character"].isInt() ?
                            range["start"]["character"].asInt() : 0);
                    }
                    if (range.has("end") && range["end"].isObject()) {
                        diag.endLine = static_cast<int>(range["end"]["line"].isInt() ?
                            range["end"]["line"].asInt() : diag.line);
                        diag.endCol = static_cast<int>(range["end"]["character"].isInt() ?
                            range["end"]["character"].asInt() : diag.col);
                    }
                }
                if (d.has("message") && d["message"].isString()) {
                    diag.message = d["message"].asString();
                }
                if (d.has("severity") && d["severity"].isInt()) {
                    diag.severity = static_cast<int>(d["severity"].asInt());
                }
                diags.push_back(std::move(diag));
            }
        }

        if (diag_callback_) {
            diag_callback_(uri, std::move(diags));
        }
    } else if (method == "initialized" || method == "window/logMessage" ||
               method == "window/showMessage" || method == "$/progress") {
        // Silently ignore common notifications
    }
}

void LspClient::processResponse(int id, const JsonValue& result,
                                const JsonValue& error) {
    // Handle initialize response
    if (!initialized_ && !error.isNull()) {
        return;
    }

    if (!initialized_ && !result.isNull()) {
        initialized_ = true;
        // Send initialized notification
        sendMessage(buildNotification("initialized", JsonValue(JsonObject{})));
        return;
    }

    std::lock_guard<std::mutex> lock(cb_mutex_);

    // Completion response
    {
        auto it = completion_cbs_.find(id);
        if (it != completion_cbs_.end()) {
            std::vector<CompletionItem> items;
            // Result can be array or {isIncomplete, items}
            const JsonValue* arr = &result;
            if (result.isObject() && result.has("items")) {
                arr = &result["items"];
            }
            if (arr->isArray()) {
                for (const auto& ci : arr->asArray()) {
                    if (!ci.isObject()) continue;
                    CompletionItem item;
                    if (ci.has("label") && ci["label"].isString())
                        item.label = ci["label"].asString();
                    if (ci.has("detail") && ci["detail"].isString())
                        item.detail = ci["detail"].asString();
                    if (ci.has("insertText") && ci["insertText"].isString())
                        item.insertText = ci["insertText"].asString();
                    else
                        item.insertText = item.label;
                    if (ci.has("kind") && ci["kind"].isInt())
                        item.kind = static_cast<int>(ci["kind"].asInt());
                    items.push_back(std::move(item));
                }
            }
            it->second(std::move(items));
            completion_cbs_.erase(it);
            return;
        }
    }

    // Hover response
    {
        auto it = hover_cbs_.find(id);
        if (it != hover_cbs_.end()) {
            std::string content;
            if (result.isObject() && result.has("contents")) {
                const auto& c = result["contents"];
                if (c.isString()) {
                    content = c.asString();
                } else if (c.isObject() && c.has("value") && c["value"].isString()) {
                    content = c["value"].asString();
                } else if (c.isArray()) {
                    for (const auto& elem : c.asArray()) {
                        if (elem.isString()) content += elem.asString() + "\n";
                        else if (elem.isObject() && elem.has("value"))
                            content += elem["value"].asString() + "\n";
                    }
                }
            }
            it->second(content);
            hover_cbs_.erase(it);
            return;
        }
    }

    // Definition response
    {
        auto it = def_cbs_.find(id);
        if (it != def_cbs_.end()) {
            std::string uri;
            int line = 0, col = 0;
            // Result can be Location or Location[]
            const JsonValue* loc = &result;
            if (result.isArray() && !result.asArray().empty()) {
                loc = &result.asArray()[0];
            }
            if (loc->isObject()) {
                if (loc->has("uri") && (*loc)["uri"].isString())
                    uri = (*loc)["uri"].asString();
                if (loc->has("range") && (*loc)["range"].isObject()) {
                    const auto& start = (*loc)["range"]["start"];
                    if (start.isObject()) {
                        if (start.has("line") && start["line"].isInt())
                            line = static_cast<int>(start["line"].asInt());
                        if (start.has("character") && start["character"].isInt())
                            col = static_cast<int>(start["character"].asInt());
                    }
                }
            }
            it->second(uri, line, col);
            def_cbs_.erase(it);
            return;
        }
    }
}

// ---- Public LSP methods ----

std::string LspClient::uriFromPath(const std::string& path) {
    if (path.substr(0, 7) == "file://") return path;
    std::string uri = "file://";
    for (char c : path) {
        if (c == ' ') uri += "%20";
        else uri += c;
    }
    return uri;
}

void LspClient::didOpen(const std::string& uri, const std::string& languageId,
                        const std::string& text, int version) {
    if (!running_) return;
    JsonObject params;
    JsonObject doc;
    doc["uri"] = uriFromPath(uri);
    doc["languageId"] = languageId;
    doc["version"] = static_cast<int64_t>(version);
    doc["text"] = text;
    params["textDocument"] = JsonValue(std::move(doc));
    sendMessage(buildNotification("textDocument/didOpen", JsonValue(std::move(params))));
}

void LspClient::didChange(const std::string& uri, const std::string& text, int version) {
    if (!running_) return;
    JsonObject params;
    JsonObject doc;
    doc["uri"] = uriFromPath(uri);
    doc["version"] = static_cast<int64_t>(version);
    params["textDocument"] = JsonValue(std::move(doc));

    JsonArray changes;
    JsonObject change;
    change["text"] = text;
    changes.push_back(JsonValue(std::move(change)));
    params["contentChanges"] = JsonValue(std::move(changes));
    sendMessage(buildNotification("textDocument/didChange", JsonValue(std::move(params))));
}

void LspClient::didClose(const std::string& uri) {
    if (!running_) return;
    JsonObject params;
    JsonObject doc;
    doc["uri"] = uriFromPath(uri);
    params["textDocument"] = JsonValue(std::move(doc));
    sendMessage(buildNotification("textDocument/didClose", JsonValue(std::move(params))));
}

void LspClient::didSave(const std::string& uri) {
    if (!running_) return;
    JsonObject params;
    JsonObject doc;
    doc["uri"] = uriFromPath(uri);
    params["textDocument"] = JsonValue(std::move(doc));
    sendMessage(buildNotification("textDocument/didSave", JsonValue(std::move(params))));
}

void LspClient::requestCompletion(const std::string& uri, int line, int col,
                                  CompletionCallback cb) {
    if (!running_ || !initialized_) return;
    int id = next_id_++;
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        completion_cbs_[id] = std::move(cb);
    }
    JsonObject params;
    JsonObject doc;
    doc["uri"] = uriFromPath(uri);
    params["textDocument"] = JsonValue(std::move(doc));

    JsonObject pos;
    pos["line"] = static_cast<int64_t>(line);
    pos["character"] = static_cast<int64_t>(col);
    params["position"] = JsonValue(std::move(pos));

    sendMessage(buildRequest(id, "textDocument/completion", JsonValue(std::move(params))));
}

void LspClient::requestHover(const std::string& uri, int line, int col,
                             HoverCallback cb) {
    if (!running_ || !initialized_) return;
    int id = next_id_++;
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        hover_cbs_[id] = std::move(cb);
    }
    JsonObject params;
    JsonObject doc;
    doc["uri"] = uriFromPath(uri);
    params["textDocument"] = JsonValue(std::move(doc));

    JsonObject pos;
    pos["line"] = static_cast<int64_t>(line);
    pos["character"] = static_cast<int64_t>(col);
    params["position"] = JsonValue(std::move(pos));

    sendMessage(buildRequest(id, "textDocument/hover", JsonValue(std::move(params))));
}

void LspClient::requestDefinition(const std::string& uri, int line, int col,
                                  DefinitionCallback cb) {
    if (!running_ || !initialized_) return;
    int id = next_id_++;
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        def_cbs_[id] = std::move(cb);
    }
    JsonObject params;
    JsonObject doc;
    doc["uri"] = uriFromPath(uri);
    params["textDocument"] = JsonValue(std::move(doc));

    JsonObject pos;
    pos["line"] = static_cast<int64_t>(line);
    pos["character"] = static_cast<int64_t>(col);
    params["position"] = JsonValue(std::move(pos));

    sendMessage(buildRequest(id, "textDocument/definition", JsonValue(std::move(params))));
}

} // namespace xenon::lsp
