#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <vector>
#include "lsp/json_rpc.hpp"

namespace xenon::lsp {

struct Diagnostic {
    int line = 0;       // 0-based
    int col = 0;        // 0-based
    int endLine = 0;
    int endCol = 0;
    std::string message;
    int severity = 1;   // 1=Error 2=Warning 3=Info 4=Hint
};

struct CompletionItem {
    std::string label;
    std::string detail;
    std::string insertText;
    int kind = 0;
};

using DiagnosticsCallback = std::function<void(const std::string& uri,
                                               std::vector<Diagnostic>)>;
using CompletionCallback  = std::function<void(std::vector<CompletionItem>)>;
using HoverCallback       = std::function<void(const std::string& content)>;
using DefinitionCallback  = std::function<void(const std::string& uri, int line, int col)>;

class LspClient {
public:
    LspClient();
    ~LspClient();

    // Launch the language server process.
    // command: e.g. {"clangd"} or {"rust-analyzer"}
    bool start(const std::vector<std::string>& command,
               const std::string& rootUri);

    void stop();
    bool isRunning() const { return running_; }
    bool isInitialized() const { return initialized_; }

    // LSP protocol methods
    void didOpen(const std::string& uri, const std::string& languageId,
                 const std::string& text, int version = 1);
    void didChange(const std::string& uri, const std::string& text, int version);
    void didClose(const std::string& uri);
    void didSave(const std::string& uri);

    void requestCompletion(const std::string& uri, int line, int col,
                           CompletionCallback cb);
    void requestHover(const std::string& uri, int line, int col,
                      HoverCallback cb);
    void requestDefinition(const std::string& uri, int line, int col,
                           DefinitionCallback cb);

    // Set diagnostics receiver (called from reader thread, dispatch to main)
    void setDiagnosticsCallback(DiagnosticsCallback cb) {
        diag_callback_ = std::move(cb);
    }

private:
    void sendMessage(const std::string& json);
    void readerThread();
    void processMessage(const JsonValue& msg);
    void processNotification(const std::string& method, const JsonValue& params);
    void processResponse(int id, const JsonValue& result, const JsonValue& error);

    std::string uriFromPath(const std::string& path);

    // Process I/O
    int server_stdin_fd_ = -1;
    int server_stdout_fd_ = -1;
    pid_t server_pid_ = -1;

    std::thread reader_thread_;
    std::atomic<bool> running_{false};

    std::mutex send_mutex_;
    std::mutex cb_mutex_;
    std::atomic<int> next_id_{1};

    // Pending response callbacks
    std::unordered_map<int, CompletionCallback> completion_cbs_;
    std::unordered_map<int, HoverCallback> hover_cbs_;
    std::unordered_map<int, DefinitionCallback> def_cbs_;

    DiagnosticsCallback diag_callback_;

    // Read buffer
    std::string read_buf_;
    bool initialized_ = false;
};

} // namespace xenon::lsp
