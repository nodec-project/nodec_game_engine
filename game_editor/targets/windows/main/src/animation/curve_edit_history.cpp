#include "curve_edit_history.hpp"

#include <algorithm>
#include <chrono>

namespace nodec_game_editor {
namespace animation {

using namespace nodec_animation;

CurveEditHistory::CurveEditHistory(size_t max_history_size)
    : max_history_size_(max_history_size) {
}

void CurveEditHistory::add_edit(
    const std::string& property_path,
    const AnimationCurve& before,
    const AnimationCurve& after) {
    
    // Clear any redo history when adding new edit
    if (current_index_ < history_.size()) {
        history_.erase(history_.begin() + current_index_, history_.end());
    }
    
    // Create new edit entry
    auto edit = std::make_shared<CurveEdit>();
    edit->property_path = property_path;
    edit->before_curve = before;
    edit->after_curve = after;
    edit->timestamp = std::chrono::system_clock::now();
    
    // Add to history
    history_.push_back(edit);
    
    // Trim history if it exceeds max size
    if (history_.size() > max_history_size_) {
        history_.erase(history_.begin());
    } else {
        current_index_++;
    }
}

bool CurveEditHistory::can_undo() const {
    return current_index_ > 0 && !history_.empty();
}

bool CurveEditHistory::can_redo() const {
    return current_index_ < history_.size();
}

std::shared_ptr<CurveEdit> CurveEditHistory::undo() {
    if (!can_undo()) {
        return nullptr;
    }
    
    current_index_--;
    return history_[current_index_];
}

std::shared_ptr<CurveEdit> CurveEditHistory::redo() {
    if (!can_redo()) {
        return nullptr;
    }
    
    auto edit = history_[current_index_];
    current_index_++;
    return edit;
}

void CurveEditHistory::clear() {
    history_.clear();
    current_index_ = 0;
}

} // namespace animation
} // namespace nodec_game_editor