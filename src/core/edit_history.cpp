#include "core/edit_history.hpp"

namespace xenon::core {

EditHistory::EditHistory(size_t maxCommands)
    : max_commands_(maxCommands) {
}

void EditHistory::execute(std::unique_ptr<EditCommand> command) {
    if (!command) {
        return;
    }

    commands_.erase(commands_.begin() + current_index_, commands_.end());
    command->execute();
    commands_.push_back(std::move(command));
    current_index_++;

    if (commands_.size() > max_commands_) {
        commands_.erase(commands_.begin());
        current_index_--;
    }
}

void EditHistory::undo() {
    if (!canUndo()) {
        return;
    }

    current_index_--;
    commands_[current_index_]->undo();
}

void EditHistory::redo() {
    if (!canRedo()) {
        return;
    }

    commands_[current_index_]->execute();
    current_index_++;
}

void EditHistory::clear() {
    commands_.clear();
    current_index_ = 0;
}

bool EditHistory::canUndo() const {
    return current_index_ > 0;
}

bool EditHistory::canRedo() const {
    return current_index_ < commands_.size();
}

} // namespace xenon::core
