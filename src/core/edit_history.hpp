#pragma once

#include <memory>
#include <vector>
#include <string>

namespace xenon::core {

class EditCommand {
public:
    virtual ~EditCommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string description() const = 0;
};

class EditHistory {
public:
    explicit EditHistory(size_t maxCommands = 1000);

    void execute(std::unique_ptr<EditCommand> command);
    void undo();
    void redo();
    void clear();

    bool canUndo() const;
    bool canRedo() const;

    size_t undoCount() const { return current_index_; }
    size_t redoCount() const { return commands_.size() - current_index_; }

private:
    std::vector<std::unique_ptr<EditCommand>> commands_;
    size_t current_index_ = 0;
    size_t max_commands_;
};

} // namespace xenon::core
