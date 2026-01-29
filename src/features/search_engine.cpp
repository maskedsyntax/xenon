#include "features/search_engine.hpp"
#include <algorithm>
#include <cctype>

namespace xenon::features {

std::vector<SearchResult> SearchEngine::findAll(
    const std::string& text,
    const std::string& pattern,
    bool caseSensitive) {
    std::vector<SearchResult> results;

    if (pattern.empty() || pattern.length() > text.length()) {
        return results;
    }

    std::string searchText = text;
    std::string searchPattern = pattern;

    if (!caseSensitive) {
        std::transform(searchText.begin(), searchText.end(), searchText.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        std::transform(searchPattern.begin(), searchPattern.end(), searchPattern.begin(),
                      [](unsigned char c) { return std::tolower(c); });
    }

    size_t pos = 0;
    while ((pos = searchText.find(searchPattern, pos)) != std::string::npos) {
        results.emplace_back(SearchResult{pos, searchPattern.length()});
        pos++;
    }

    return results;
}

SearchResult SearchEngine::findNext(
    const std::string& text,
    const std::string& pattern,
    size_t startOffset,
    bool caseSensitive) {
    auto results = findAll(text, pattern, caseSensitive);

    for (const auto& result : results) {
        if (result.offset >= startOffset) {
            return result;
        }
    }

    return SearchResult{std::string::npos, 0};
}

SearchResult SearchEngine::findPrevious(
    const std::string& text,
    const std::string& pattern,
    size_t startOffset,
    bool caseSensitive) {
    auto results = findAll(text, pattern, caseSensitive);

    // Find the last result that ends before or at startOffset? 
    // Usually "Previous" means result strictly BEFORE the cursor.
    
    if (results.empty()) {
        return SearchResult{std::string::npos, 0};
    }

    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        if (it->offset < startOffset) {
            return *it;
        }
    }

    return SearchResult{std::string::npos, 0};
}

} // namespace xenon::features
