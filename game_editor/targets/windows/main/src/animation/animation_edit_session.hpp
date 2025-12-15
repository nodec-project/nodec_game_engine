#ifndef NODEC_GAME_EDITOR__ANIMATION__ANIMATION_EDIT_SESSION_HPP_
#define NODEC_GAME_EDITOR__ANIMATION__ANIMATION_EDIT_SESSION_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>

#include <nodec/entities/entity.hpp>
#include <nodec_animation/resources/animation_clip.hpp>
#include <nodec_animation/animation_curve.hpp>
#include <nodec_world/world.hpp>

// Include the full header for unique_ptr
#include "curve_edit_history.hpp"

namespace nodec_game_editor {
namespace animation {

/**
 * @brief Represents an editing session for an AnimationClip
 * 
 * A session manages:
 * - Original and working copies of the clip
 * - Change tracking for all curves
 * - Undo/redo history
 * - Preview state
 */
class AnimationEditSession {
public:
    using AnimationClip = nodec_animation::resources::AnimationClip;
    using AnimationCurve = nodec_animation::AnimationCurve;
    using Keyframe = nodec_animation::Keyframe;
    
    struct SessionInfo {
        std::string session_id;
        nodec::entities::Entity target_entity;
        std::string clip_path;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_modified;
        bool is_dirty;
    };
    
    struct CurveState {
        std::string property_path;
        AnimationCurve original_curve;
        AnimationCurve current_curve;
        bool is_modified;
        
        bool has_changes() const {
            return is_modified;
        }
    };

public:
    /**
     * @brief Create a new editing session
     * @param session_id Unique identifier for this session
     * @param entity Target entity with Animator component
     * @param clip Animation clip to edit (can be null for new clip)
     * @param clip_path Resource path of the clip
     */
    AnimationEditSession(
        const std::string& session_id,
        nodec::entities::Entity entity,
        std::shared_ptr<AnimationClip> clip,
        const std::string& clip_path = ""
    );
    
    ~AnimationEditSession() = default;
    
    // Session info
    const std::string& get_session_id() const { return session_info_.session_id; }
    nodec::entities::Entity get_target_entity() const { return session_info_.target_entity; }
    const SessionInfo& get_info() const { return session_info_; }
    bool is_dirty() const { return session_info_.is_dirty; }
    
    // Clip access
    std::shared_ptr<AnimationClip> get_original_clip() const { return original_clip_; }
    std::shared_ptr<AnimationClip> get_working_clip() const { return working_clip_; }
    
    // Curve editing
    bool update_curve(const std::string& property_path, const std::vector<Keyframe>& keyframes);
    bool add_curve(const std::string& property_path, const std::string& component_type = "");
    bool remove_curve(const std::string& property_path);
    
    // Get curve state
    const CurveState* get_curve_state(const std::string& property_path) const;
    std::vector<std::string> get_modified_curves() const;
    std::unordered_map<std::string, CurveState> get_all_curve_states() const { return curve_states_; }
    
    // Entity hierarchy helpers
    std::string build_entity_path(nodec::entities::Entity entity, nodec::entities::Entity root) const;
    
    // Undo/Redo
    bool can_undo() const;
    bool can_redo() const;
    bool undo();
    bool redo();
    void clear_history();
    
    // Session operations
    bool save(const std::string& save_path = "");
    bool revert();
    
    // Preview
    void set_preview_enabled(bool enabled) { preview_enabled_ = enabled; }
    bool is_preview_enabled() const { return preview_enabled_; }
    void set_current_time(float time) { current_time_ = time; }
    float get_current_time() const { return current_time_; }
    
    // Apply animation at current time
    void apply_to_preview(nodec_world::World* world);
    
    // Get animation duration
    float get_duration() const;
    
private:
    // Helper to find curve in clip hierarchy
    AnimationCurve* find_curve_by_path(
        nodec_animation::resources::AnimatedEntity& entity,
        const std::string& property_path,
        const std::string& current_path = ""
    );
    
    const AnimationCurve* find_curve_by_path(
        const nodec_animation::resources::AnimatedEntity& entity,
        const std::string& property_path,
        const std::string& current_path = ""
    ) const;
    
    // Parse property path (e.g., "child/Transform/position.x")
    struct ParsedPropertyPath {
        std::string entity_path;      // "child" or empty for root
        std::string component_type;   // "Transform"
        std::string property_name;    // "position.x"
    };
    ParsedPropertyPath parse_property_path(const std::string& path) const;
    
    // Update internal state
    void mark_dirty();
    void update_curve_state(const std::string& property_path);
    
    // History management
    void push_to_history(const std::string& property_path, const AnimationCurve& before);
    
private:
    SessionInfo session_info_;
    
    // Clip management
    std::shared_ptr<AnimationClip> original_clip_;
    std::shared_ptr<AnimationClip> working_clip_;
    
    // Curve states
    std::unordered_map<std::string, CurveState> curve_states_;
    std::unordered_set<std::string> modified_curves_;
    
    // History
    std::unique_ptr<CurveEditHistory> history_;
    
    // Preview state
    bool preview_enabled_ = true;
    float current_time_ = 0.0f;
};

} // namespace animation
} // namespace nodec_game_editor

#endif // NODEC_GAME_EDITOR__ANIMATION__ANIMATION_EDIT_SESSION_HPP_