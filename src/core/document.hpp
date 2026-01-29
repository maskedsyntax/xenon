#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "core/text_position.hpp"
#include "core/text_range.hpp"

namespace xenon::core {

class Document {
public:
    using ModifiedCallback = std::function<void()>;
    using ChangedCallback = std::function<void(size_t pos, size_t oldLen, size_t newLen)>;

    Document();
    ~Document() = default;

    std::string text() const;
    std::string text(const TextRange& range) const;
    char charAt(size_t position) const;
    size_t length() const;

    void insert(size_t position, std::string_view text);
    void erase(const TextRange& range);
    void replace(const TextRange& range, std::string_view text);
    void clear();

    size_t lineCount() const;
    size_t lineLength(size_t line) const;
    std::string lineText(size_t line) const;
    size_t offsetFromPosition(const TextPosition& position) const;
    TextPosition positionFromOffset(size_t offset) const;

    const std::string& encoding() const { return encoding_; }
    void setEncoding(std::string_view encoding) { encoding_ = encoding; }

    const std::string& lineEnding() const { return line_ending_; }
    void setLineEnding(std::string_view ending) { line_ending_ = ending; }

    bool isModified() const { return is_modified_; }
    void setModified(bool modified) { is_modified_ = modified; }
    void resetModification() { is_modified_ = false; }

    void onModified(const ModifiedCallback& callback) { modified_callbacks_.push_back(callback); }
    void onChanged(const ChangedCallback& callback) { changed_callbacks_.push_back(callback); }

private:
    std::string buffer_;
    std::string encoding_ = "UTF-8";
    std::string line_ending_ = "\n";
    bool is_modified_ = false;
    mutable std::vector<size_t> line_offsets_;
    mutable bool line_offsets_valid_ = false;

    void notifyModified();
    void notifyChanged(size_t pos, size_t oldLen, size_t newLen);
    void invalidateLineOffsets();
    void rebuildLineOffsets() const;

    std::vector<ModifiedCallback> modified_callbacks_;
    std::vector<ChangedCallback> changed_callbacks_;
};

} // namespace xenon::core
