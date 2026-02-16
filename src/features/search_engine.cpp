#include "features/search_engine.hpp"
#include <algorithm>
#include <cctype>
#include <regex>

namespace xenon::features {

std::vector<SearchResult> SearchEngine::findAll(
    const std::string& text,
    const std::string& pattern,
    bool caseSensitive,
    bool useRegex) {
    std::vector<SearchResult> results;

    if (pattern.empty() || text.empty()) {
        return results;
    }

    if (useRegex) {
        try {
            auto flags = std::regex_constants::ECMAScript;
            if (!caseSensitive) {
                flags |= std::regex_constants::icase;
            }
            std::regex re(pattern, flags);
            auto begin = std::sregex_iterator(text.begin(), text.end(), re);
            auto end = std::sregex_iterator();

            for (auto it = begin; it != end; ++it) {
                auto matchOffset = static_cast<size_t>(it->position());
                auto matchLength = static_cast<size_t>(it->length());
                if (matchLength == 0) break; // Avoid infinite loop on zero-length matches
                results.emplace_back(SearchResult{matchOffset, matchLength});
            }
        } catch (const std::regex_error&) {
            // Invalid regex pattern, return empty results
        }
        return results;
    }

    // Literal search
    if (pattern.length() > text.length()) {
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
    bool caseSensitive,
    bool useRegex) {
    auto results = findAll(text, pattern, caseSensitive, useRegex);

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
    bool caseSensitive,
    bool useRegex) {
    auto results = findAll(text, pattern, caseSensitive, useRegex);

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
