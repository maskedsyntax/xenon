#pragma once

#include "core/text_position.hpp"

namespace xenon::core {

class TextRange {
public:
    TextRange() = default;
    explicit TextRange(const TextPosition& start, const TextPosition& end);

    const TextPosition& start() const { return start_; }
    const TextPosition& end() const { return end_; }

    void setStart(const TextPosition& position) { start_ = position; }
    void setEnd(const TextPosition& position) { end_ = position; }

    bool isEmpty() const { return start_ == end_; }
    bool contains(const TextPosition& position) const;
    bool intersects(const TextRange& range) const;

private:
    TextPosition start_;
    TextPosition end_;

    void normalize();
};

} // namespace xenon::core
