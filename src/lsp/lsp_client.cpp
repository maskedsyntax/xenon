#include "lsp/lsp_client.hpp"
#include <QJsonDocument>
#include <QDebug>
#include <QCoreApplication>

namespace xenon::lsp {

LspClient::LspClient(QObject* parent) : QObject(parent) {
    connect(&process_, &QProcess::readyReadStandardOutput, this, &LspClient::onReadyRead);
    connect(&process_, &QProcess::errorOccurred, this, &LspClient::onProcessError);
    connect(&process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LspClient::onProcessFinished);
}

LspClient::~LspClient() {
    stop();
}

bool LspClient::start(const QStringList& command, const QString& rootUri) {
    if (process_.state() != QProcess::NotRunning) return false;

    process_.start(command.first(), command.mid(1));
    if (!process_.waitForStarted()) return false;

    // Initialize
    QJsonObject params;
    params["processId"] = QCoreApplication::applicationPid();
    params["rootUri"] = rootUri;
    params["capabilities"] = QJsonObject(); // Basic empty capabilities

    QJsonObject init_msg;
    init_msg["jsonrpc"] = "2.0";
    init_msg["id"] = next_id_++;
    init_msg["method"] = "initialize";
    init_msg["params"] = params;

    sendMessage(init_msg);
    return true;
}

void LspClient::stop() {
    if (process_.state() == QProcess::Running) {
        process_.terminate();
        if (!process_.waitForFinished(2000)) {
            process_.kill();
        }
    }
}

void LspClient::sendMessage(const QJsonObject& msg) {
    QByteArray body = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    QByteArray header = QString("Content-Length: %1\r\n\r\n").arg(body.size()).toUtf8();
    process_.write(header);
    process_.write(body);
}

void LspClient::onReadyRead() {
    read_buffer_.append(process_.readAllStandardOutput());

    while (true) {
        qsizetype header_end = read_buffer_.indexOf("\r\n\r\n");
        if (header_end == -1) break;

        QString header = QString::fromUtf8(read_buffer_.left(header_end));
        int content_length = 0;
        for (const QString& line : header.split("\r\n")) {
            if (line.startsWith("Content-Length:", Qt::CaseInsensitive)) {
                content_length = line.mid(15).trimmed().toInt();
            }
        }

        if (read_buffer_.size() < header_end + 4 + content_length) break;

        QByteArray body = read_buffer_.mid(header_end + 4, content_length);
        read_buffer_.remove(0, header_end + 4 + content_length);

        processMessage(body);
    }
}

void LspClient::processMessage(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject msg = doc.object();
    if (msg.contains("method")) {
        handleNotification(msg["method"].toString(), msg["params"]);
    } else if (msg.contains("id")) {
        handleResponse(msg["id"].toInt(), msg["result"], msg["error"]);
    }
}

void LspClient::handleNotification(const QString& method, const QJsonValue& params) {
    if (method == "textDocument/publishDiagnostics") {
        QJsonObject p = params.toObject();
        QString uri = p["uri"].toString();
        QJsonArray diags = p["diagnostics"].toArray();
        QList<Diagnostic> result;
        for (const auto& d_val : diags) {
            QJsonObject d = d_val.toObject();
            Diagnostic diag;
            QJsonObject range = d["range"].toObject();
            diag.line = range["start"].toObject()["line"].toInt();
            diag.col = range["start"].toObject()["character"].toInt();
            diag.message = d["message"].toString();
            diag.severity = d["severity"].toInt();
            result.append(diag);
        }
        emit diagnosticsReceived(uri, result);
    }
}

void LspClient::handleResponse(int id, const QJsonValue& result, const QJsonValue& error) {
    Q_UNUSED(id);
    Q_UNUSED(result);
    if (!error.isNull()) {
        qDebug() << "LSP Error:" << error;
        return;
    }
    // Handle responses for completion, definition etc.
}

void LspClient::didOpen(const QString& uri, const QString& languageId, const QString& text, int version) {
    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = uri;
    doc["languageId"] = languageId;
    doc["version"] = version;
    doc["text"] = text;
    params["textDocument"] = doc;

    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"] = "textDocument/didOpen";
    msg["params"] = params;
    sendMessage(msg);
}

void LspClient::didChange(const QString& uri, const QString& text, int version) {
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}, {"version", version}};
    QJsonArray changes;
    changes.append(QJsonObject{{"text", text}});
    params["contentChanges"] = changes;

    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"] = "textDocument/didChange";
    msg["params"] = params;
    sendMessage(msg);
}

void LspClient::didClose(const QString& uri) {
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"] = "textDocument/didClose";
    msg["params"] = QJsonObject{{"textDocument", QJsonObject{{"uri", uri}}}};
    sendMessage(msg);
}

void LspClient::didSave(const QString& uri) {
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"] = "textDocument/didSave";
    msg["params"] = QJsonObject{{"textDocument", QJsonObject{{"uri", uri}}}};
    sendMessage(msg);
}

void LspClient::onProcessError(QProcess::ProcessError error) {
    qDebug() << "LSP Process Error:" << error;
}

void LspClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug() << "LSP Process Finished:" << exitCode << exitStatus;
    initialized_ = false;
}

} // namespace xenon::lsp
