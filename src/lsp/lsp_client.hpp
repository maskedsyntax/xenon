#pragma once

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace xenon::lsp {

struct Diagnostic {
    int line = 0;
    int col = 0;
    int endLine = 0;
    int endCol = 0;
    QString message;
    int severity = 1;
};

struct CompletionItem {
    QString label;
    QString detail;
    QString insertText;
    int kind = 0;
};

class LspClient : public QObject {
    Q_OBJECT

public:
    explicit LspClient(QObject* parent = nullptr);
    ~LspClient() override;

    bool start(const QStringList& command, const QString& rootUri);
    void stop();

    bool isRunning() const { return process_.state() == QProcess::Running; }
    bool isInitialized() const { return initialized_; }

    // LSP Methods
    void didOpen(const QString& uri, const QString& languageId, const QString& text, int version = 1);
    void didChange(const QString& uri, const QString& text, int version);
    void didClose(const QString& uri);
    void didSave(const QString& uri);

    int completion(const QString& uri, int line, int col);
    int definition(const QString& uri, int line, int col);

signals:
    void diagnosticsReceived(const QString& uri, const QList<Diagnostic>& diagnostics);
    void completionReceived(int id, const QList<CompletionItem>& items);
    void hoverReceived(int id, const QString& content);
    void definitionReceived(int id, const QString& uri, int line, int col);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void sendMessage(const QJsonObject& msg);
    void processMessage(const QByteArray& data);
    void handleNotification(const QString& method, const QJsonValue& params);
    void handleResponse(int id, const QJsonValue& result, const QJsonValue& error);

    QProcess process_;
    QByteArray read_buffer_;
    int next_id_ = 1;
    bool initialized_ = false;

    enum class RequestType {
        Initialize,
        Completion,
        Definition
    };
    std::unordered_map<int, RequestType> pending_requests_;
};

} // namespace xenon::lsp
