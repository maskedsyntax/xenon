#include "core/document.hpp"
#include <algorithm>

namespace xenon::core {

Document::Document() {
    rebuildLineOffsets();
}

std::string Document::text() const {
    return buffer_;
}

std::string Document::text(const TextRange& range) const {
    const size_t startOffset = offsetFromPosition(range.start());
    const size_t endOffset = offsetFromPosition(range.end());

    if (startOffset >= buffer_.size() || endOffset <= startOffset) {
        return "";
    }

    return buffer_.substr(startOffset, endOffset - startOffset);
}

char Document::charAt(size_t position) const {
    if (position >= buffer_.size()) {
        return '\0';
    }
    return buffer_[position];
}

size_t Document::length() const {
    return buffer_.size();
}

void Document::insert(size_t position, std::string_view text) {
    if (text.empty() || position > buffer_.size()) {
        return;
    }

    buffer_.insert(position, text);
    invalidateLineOffsets();
    notifyChanged(position, 0, text.length());
    notifyModified();
}

void Document::erase(const TextRange& range) {
    const size_t startOffset = offsetFromPosition(range.start());
    const size_t endOffset = offsetFromPosition(range.end());

    if (startOffset >= buffer_.size() || endOffset <= startOffset) {
        return;
    }

    const size_t eraseLen = endOffset - startOffset;
    buffer_.erase(startOffset, eraseLen);

    invalidateLineOffsets();
    notifyChanged(startOffset, eraseLen, 0);
    notifyModified();
}

void Document::replace(const TextRange& range, std::string_view text) {
    const size_t startOffset = offsetFromPosition(range.start());
    const size_t endOffset = offsetFromPosition(range.end());

    if (startOffset >= buffer_.size() || endOffset < startOffset) {
        return;
    }

    const size_t eraseLen = std::min(endOffset, buffer_.size()) - startOffset;
    buffer_.erase(startOffset, eraseLen);
    buffer_.insert(startOffset, text);

    invalidateLineOffsets();
    notifyChanged(startOffset, eraseLen, text.length());
    notifyModified();
}

void Document::clear() {
    buffer_.clear();
    invalidateLineOffsets();
    notifyChanged(0, buffer_.size(), 0);
    notifyModified();
}

size_t Document::lineCount() const {
    if (buffer_.empty()) {
        return 1;
    }

    size_t count = 1;
    for (char c : buffer_) {
        if (c == '\n') {
            count++;
        }
    }
    return count;
}

size_t Document::lineLength(size_t line) const {
    if (line >= lineCount()) {
        return 0;
    }

    if (!line_offsets_valid_) {
        rebuildLineOffsets();
    }

    const size_t start = line_offsets_[line];
    const size_t end = (line + 1 < line_offsets_.size()) ? line_offsets_[line + 1] : buffer_.size();

    size_t len = end - start;
    if (len > 0 && buffer_[end - 1] == '\n') {
        len--;
    }

    return len;
}

std::string Document::lineText(size_t line) const {
    if (line >= lineCount()) {
        return "";
    }

    if (!line_offsets_valid_) {
        rebuildLineOffsets();
    }

    const size_t start = line_offsets_[line];
    const size_t end = (line + 1 < line_offsets_.size()) ? line_offsets_[line + 1] : buffer_.size();

    std::string result = buffer_.substr(start, end - start);
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

size_t Document::offsetFromPosition(const TextPosition& position) const {
    if (!line_offsets_valid_) {
        rebuildLineOffsets();
    }

    if (position.line() >= line_offsets_.size()) {
        return buffer_.size();
    }

    const size_t lineStart = line_offsets_[position.line()];
    return std::min(lineStart + position.column(), buffer_.size());
}

TextPosition Document::positionFromOffset(size_t offset) const {
    if (!line_offsets_valid_) {
        rebuildLineOffsets();
    }

    if (offset >= buffer_.size()) {
        return TextPosition(lineCount() - 1, 0);
    }

    size_t line = 0;
    for (size_t i = 0; i < line_offsets_.size(); i++) {
        if (line_offsets_[i] > offset) {
            line = i - 1;
            break;
        }
        line = i;
    }

    const size_t lineStart = (line < line_offsets_.size()) ? line_offsets_[line] : 0;
    const size_t column = offset - lineStart;

    return TextPosition(line, column);
}

void Document::notifyModified() {
    for (const auto& callback : modified_callbacks_) {
        callback();
    }
}

void Document::notifyChanged(size_t pos, size_t oldLen, size_t newLen) {
    for (const auto& callback : changed_callbacks_) {
        callback(pos, oldLen, newLen);
    }
}

void Document::invalidateLineOffsets() {
    line_offsets_valid_ = false;
}

void Document::rebuildLineOffsets() const {
    line_offsets_.clear();
    line_offsets_.push_back(0);

    for (size_t i = 0; i < buffer_.size(); i++) {
        if (buffer_[i] == '\n') {
            line_offsets_.push_back(i + 1);
        }
    }

    line_offsets_valid_ = true;
}

} // namespace xenon::core
