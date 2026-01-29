#include "core/text_position.hpp"

namespace xenon::core {

TextPosition::TextPosition(size_t line, size_t column)
    : line_(line), column_(column) {
}

bool TextPosition::operator==(const TextPosition& other) const {
    return line_ == other.line_ && column_ == other.column_;
}

bool TextPosition::operator!=(const TextPosition& other) const {
    return !(*this == other);
}

bool TextPosition::operator<(const TextPosition& other) const {
    if (line_ != other.line_) {
        return line_ < other.line_;
    }
    return column_ < other.column_;
}

bool TextPosition::operator<=(const TextPosition& other) const {
    return *this < other || *this == other;
}

bool TextPosition::operator>(const TextPosition& other) const {
    return other < *this;
}

bool TextPosition::operator>=(const TextPosition& other) const {
    return !(*this < other);
}

} // namespace xenon::core
