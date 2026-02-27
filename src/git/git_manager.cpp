#include "git/git_manager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <algorithm>

#ifdef HAVE_LIBGIT2
#include <git2.h>
#endif

namespace fs = std::filesystem;

namespace xenon::git {

GitManager::GitManager() {
#ifdef HAVE_LIBGIT2
    git_libgit2_init();
#endif
}

GitManager::~GitManager() {
#ifdef HAVE_LIBGIT2
    if (repo_) {
        git_repository_free(repo_);
        repo_ = nullptr;
    }
    git_libgit2_shutdown();
#endif
}

bool GitManager::setWorkingDirectory(const std::string& path) {
    working_dir_ = path;
    branch_ = "";
    is_git_repo_ = false;

#ifdef HAVE_LIBGIT2
    if (repo_) {
        git_repository_free(repo_);
        repo_ = nullptr;
    }

    int err = git_repository_discover(nullptr, path.c_str(), 0, nullptr);
    if (err != 0) return false;

    char git_path[4096] = {};
    git_buf buf = {};
    if (git_repository_discover(&buf, path.c_str(), 0, nullptr) == 0) {
        std::string discovered(buf.ptr);
        git_buf_free(&buf);
        if (git_repository_open(&repo_, discovered.c_str()) == 0) {
            is_git_repo_ = true;
            detectBranch();
        }
    }
#else
    // Fallback: check for .git directory in ancestors
    fs::path p = path;
    while (!p.empty() && p != p.parent_path()) {
        if (fs::exists(p / ".git")) {
            is_git_repo_ = true;
            break;
        }
        p = p.parent_path();
    }
    if (is_git_repo_) {
        detectBranch();
    }
#endif

    return is_git_repo_;
}

void GitManager::detectBranch() {
#ifdef HAVE_LIBGIT2
    if (!repo_) return;
    git_reference* head = nullptr;
    if (git_repository_head(&head, repo_) == 0) {
        branch_ = git_reference_shorthand(head);
        git_reference_free(head);
    } else {
        branch_ = "HEAD";
    }
#else
    // Parse .git/HEAD file
    fs::path p = working_dir_;
    while (!p.empty() && p != p.parent_path()) {
        fs::path head_path = p / ".git" / "HEAD";
        if (fs::exists(head_path)) {
            std::ifstream f(head_path);
            std::string line;
            if (std::getline(f, line)) {
                const std::string prefix = "ref: refs/heads/";
                if (line.substr(0, prefix.size()) == prefix) {
                    branch_ = line.substr(prefix.size());
                } else if (line.size() >= 7) {
                    branch_ = line.substr(0, 7); // detached HEAD
                }
            }
            break;
        }
        p = p.parent_path();
    }
#endif
}

std::string GitManager::runGitCommand(const std::vector<std::string>& args) const {
    std::string cmd = "git -C \"" + working_dir_ + "\"";
    for (const auto& arg : args) {
        cmd += " " + arg;
    }
    cmd += " 2>/dev/null";

    std::array<char, 4096> buf{};
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        result += buf.data();
    }
    pclose(pipe);
    return result;
}

std::vector<DiffHunk> GitManager::getFileDiff(const std::string& filepath,
                                               const std::string& /*fileContent*/) const {
    std::vector<DiffHunk> hunks;
    if (!is_git_repo_) return hunks;

    // Use git diff --unified=0 to get line-level diff
    std::string output = runGitCommand({"diff", "--unified=0", "--", "\"" + filepath + "\""});

    // Parse unified diff output
    std::istringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.size() < 3) continue;
        // @@ -old_start,old_count +new_start,new_count @@
        if (line.substr(0, 3) == "@@ ") {
            // Parse hunk header
            size_t plus_pos = line.find(" +");
            if (plus_pos == std::string::npos) continue;
            size_t space_after = line.find(' ', plus_pos + 2);
            if (space_after == std::string::npos) continue;

            std::string new_range = line.substr(plus_pos + 2, space_after - plus_pos - 2);
            size_t comma = new_range.find(',');
            int new_start = 0, new_count = 1;
            if (comma != std::string::npos) {
                new_start = std::stoi(new_range.substr(0, comma));
                new_count = std::stoi(new_range.substr(comma + 1));
            } else {
                new_start = std::stoi(new_range);
            }

            // Also parse old range to detect deletions
            size_t minus_pos = line.find(" -");
            int old_count = 0;
            if (minus_pos != std::string::npos) {
                size_t space_m = line.find(' ', minus_pos + 2);
                std::string old_range = line.substr(minus_pos + 2,
                    space_m - minus_pos - 2);
                size_t c = old_range.find(',');
                if (c != std::string::npos) {
                    old_count = std::stoi(old_range.substr(c + 1));
                } else {
                    old_count = 1;
                }
            }

            DiffHunk hunk;
            hunk.start_line = std::max(0, new_start - 1);
            hunk.count = new_count;

            if (new_count == 0 && old_count > 0) {
                hunk.type = DiffLineType::Deleted;
                hunk.count = 1; // Mark deletion at insertion point
            } else if (old_count == 0 && new_count > 0) {
                hunk.type = DiffLineType::Added;
            } else {
                hunk.type = DiffLineType::Modified;
            }

            hunks.push_back(hunk);
        }
    }

    return hunks;
}

std::string GitManager::statusSummary(const std::string& filepath,
                                       const std::string& fileContent) const {
    auto hunks = getFileDiff(filepath, fileContent);
    int added = 0, modified = 0, deleted = 0;
    for (const auto& h : hunks) {
        switch (h.type) {
            case DiffLineType::Added:    added    += h.count; break;
            case DiffLineType::Modified: modified += h.count; break;
            case DiffLineType::Deleted:  deleted  += 1;      break;
            default: break;
        }
    }

    std::string result;
    if (added)    result += "+" + std::to_string(added)    + " ";
    if (modified) result += "~" + std::to_string(modified) + " ";
    if (deleted)  result += "-" + std::to_string(deleted);
    while (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

} // namespace xenon::git
