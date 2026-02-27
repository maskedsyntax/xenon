#pragma once

#include <string>
#include <vector>
#include <functional>

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
    int start_line;  // 0-based line in the new file
    int count;
    DiffLineType type;
};

class GitManager {
public:
    GitManager();
    ~GitManager();

    // Set the working directory; returns true if it's a git repo
    bool setWorkingDirectory(const std::string& path);

    bool isGitRepo() const { return is_git_repo_; }

    // Current branch name (e.g. "main")
    std::string currentBranch() const { return branch_; }

    // Diff hunks for a file (path relative to repo root or absolute)
    std::vector<DiffHunk> getFileDiff(const std::string& filepath,
                                      const std::string& fileContent) const;

    // Short status string like "+3 ~2 -1"
    std::string statusSummary(const std::string& filepath,
                              const std::string& fileContent) const;

private:
    std::string working_dir_;
    std::string branch_;
    bool is_git_repo_ = false;

    void detectBranch();

    // Fallback: run 'git' CLI when libgit2 is not available
    std::string runGitCommand(const std::vector<std::string>& args) const;

#ifdef HAVE_LIBGIT2
    ::git_repository* repo_ = nullptr;
#endif
};

} // namespace xenon::git
