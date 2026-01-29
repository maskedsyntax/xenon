#include "core/text_range.hpp"
#include <algorithm>

namespace xenon::core {

TextRange::TextRange(const TextPosition& start, const TextPosition& end)
    : start_(start), end_(end) {
    normalize();
}

void TextRange::normalize() {
    if (end_ < start_) {
        std::swap(start_, end_);
    }
}

bool TextRange::contains(const TextPosition& position) const {
    return position >= start_ && position <= end_;
}

bool TextRange::intersects(const TextRange& range) const {
    return !(end_ < range.start_ || range.end_ < start_);
}

} // namespace xenon::core
