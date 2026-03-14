#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <vector>

#ifdef HAVE_LIBGIT2
struct git_repository;
#endif

namespace xenon::git {

enum class DiffLineType {
    Added,
    Modified,
    Deleted,
    Context,
};

struct DiffHunk {
    int start_line;
    int count;
    DiffLineType type;
};

class GitManager : public QObject {
    Q_OBJECT

public:
    explicit GitManager(QObject* parent = nullptr);
    ~GitManager() override;

    bool setWorkingDirectory(const QString& path);
    bool isGitRepo() const { return is_git_repo_; }
    QString currentBranch() const { return branch_; }

    std::vector<DiffHunk> getFileDiff(const QString& filepath, const QString& fileContent) const;
    QString statusSummary(const QString& filepath, const QString& fileContent) const;

signals:
    void branchChanged(const QString& newBranch);

private:
    void detectBranch();
    QString runGitCommand(const QStringList& args) const;

    QString working_dir_;
    QString branch_;
    bool is_git_repo_ = false;

#ifdef HAVE_LIBGIT2
    ::git_repository* repo_ = nullptr;
#endif
};

} // namespace xenon::git
