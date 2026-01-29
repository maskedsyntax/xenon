#pragma once

#include <cstddef>

namespace xenon::core {

class TextPosition {
public:
    TextPosition() = default;
    explicit TextPosition(size_t line, size_t column);

    size_t line() const { return line_; }
    size_t column() const { return column_; }

    void setLine(size_t line) { line_ = line; }
    void setColumn(size_t column) { column_ = column; }

    bool operator==(const TextPosition& other) const;
    bool operator!=(const TextPosition& other) const;
    bool operator<(const TextPosition& other) const;
    bool operator<=(const TextPosition& other) const;
    bool operator>(const TextPosition& other) const;
    bool operator>=(const TextPosition& other) const;

private:
    size_t line_ = 0;
    size_t column_ = 0;
};

} // namespace xenon::core
