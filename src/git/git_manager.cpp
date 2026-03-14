#include "git/git_manager.hpp"
#include <QDir>
#include <QDebug>

#ifdef HAVE_LIBGIT2
#include <git2.h>
#endif

namespace xenon::git {

GitManager::GitManager(QObject* parent) : QObject(parent) {
#ifdef HAVE_LIBGIT2
    git_libgit2_init();
#endif
}

GitManager::~GitManager() {
#ifdef HAVE_LIBGIT2
    if (repo_) {
        git_repository_free(repo_);
    }
    git_libgit2_shutdown();
#endif
}

bool GitManager::setWorkingDirectory(const QString& path) {
    working_dir_ = path;
    is_git_repo_ = false;

#ifdef HAVE_LIBGIT2
    if (repo_) {
        git_repository_free(repo_);
        repo_ = nullptr;
    }
    int error = git_repository_open_ext(&repo_, path.toUtf8().constData(), 0, nullptr);
    if (error == 0) {
        is_git_repo_ = true;
    }
#else
    QString output = runGitCommand({"rev-parse", "--is-inside-work-tree"});
    if (output.trimmed() == "true") {
        is_git_repo_ = true;
    }
#endif

    if (is_git_repo_) {
        detectBranch();
    } else {
        branch_ = "";
        emit branchChanged("");
    }

    return is_git_repo_;
}

void GitManager::detectBranch() {
    QString newBranch;
#ifdef HAVE_LIBGIT2
    git_reference* head = nullptr;
    if (git_repository_head(&head, repo_) == 0) {
        const char* name = git_reference_shorthand(head);
        if (name) newBranch = QString::fromUtf8(name);
        git_reference_free(head);
    }
#else
    newBranch = runGitCommand({"rev-parse", "--abbrev-ref", "HEAD"}).trimmed();
#endif

    if (branch_ != newBranch) {
        branch_ = newBranch;
        emit branchChanged(branch_);
    }
}

QString GitManager::runGitCommand(const QStringList& args) const {
    QProcess process;
    process.setWorkingDirectory(working_dir_);
    process.start("git", args);
    if (!process.waitForFinished()) return "";
    return QString::fromUtf8(process.readAllStandardOutput());
}

std::vector<DiffHunk> GitManager::getFileDiff(const QString& filepath, const QString& fileContent) const {
    Q_UNUSED(filepath);
    Q_UNUSED(fileContent);
    // TODO: Implement diff logic using libgit2 or git diff CLI
    return {};
}

QString GitManager::statusSummary(const QString& filepath, const QString& fileContent) const {
    Q_UNUSED(filepath);
    Q_UNUSED(fileContent);
    return "";
}

} // namespace xenon::git
