#ifndef NODEC_GAME_EDITOR__ANIMATION__CURVE_EDIT_HISTORY_HPP_
#define NODEC_GAME_EDITOR__ANIMATION__CURVE_EDIT_HISTORY_HPP_

#include <vector>
#include <memory>
#include <string>
#include <chrono>

#include <nodec_animation/animation_curve.hpp>

namespace nodec_game_editor {
namespace animation {

/**
 * @brief Represents a single edit operation on a curve
 */
struct CurveEdit {
    std::string property_path;
    nodec_animation::AnimationCurve before_curve;
    nodec_animation::AnimationCurve after_curve;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Manages undo/redo history for curve edits
 */
class CurveEditHistory {
public:
    CurveEditHistory(size_t max_history_size = 100);
    ~CurveEditHistory() = default;
    
    /**
     * @brief Add a new edit to the history
     */
    void add_edit(
        const std::string& property_path,
        const nodec_animation::AnimationCurve& before,
        const nodec_animation::AnimationCurve& after
    );
    
    /**
     * @brief Check if undo is available
     */
    bool can_undo() const;
    
    /**
     * @brief Check if redo is available
     */
    bool can_redo() const;
    
    /**
     * @brief Undo the last edit
     * @return The edit that was undone, or nullptr if no undo available
     */
    std::shared_ptr<CurveEdit> undo();
    
    /**
     * @brief Redo the next edit
     * @return The edit that was redone, or nullptr if no redo available
     */
    std::shared_ptr<CurveEdit> redo();
    
    /**
     * @brief Clear all history
     */
    void clear();
    
    /**
     * @brief Get the current position in history
     */
    size_t get_current_index() const { return current_index_; }
    
    /**
     * @brief Get total history size
     */
    size_t get_history_size() const { return history_.size(); }
    
private:
    std::vector<std::shared_ptr<CurveEdit>> history_;
    size_t current_index_ = 0;  // Points to the next edit position
    size_t max_history_size_;
};

} // namespace animation
} // namespace nodec_game_editor

#endif // NODEC_GAME_EDITOR__ANIMATION__CURVE_EDIT_HISTORY_HPP_