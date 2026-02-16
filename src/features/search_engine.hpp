#pragma once

#include <string>
#include <vector>
#include "core/text_range.hpp"

namespace xenon::features {

struct SearchResult {
    size_t offset;
    size_t length;
};

class SearchEngine {
public:
    static std::vector<SearchResult> findAll(
        const std::string& text,
        const std::string& pattern,
        bool caseSensitive = true,
        bool useRegex = false
    );

    static SearchResult findNext(
        const std::string& text,
        const std::string& pattern,
        size_t startOffset = 0,
        bool caseSensitive = true,
        bool useRegex = false
    );

    static SearchResult findPrevious(
        const std::string& text,
        const std::string& pattern,
        size_t startOffset = 0,
        bool caseSensitive = true,
        bool useRegex = false
    );
};

} // namespace xenon::features
