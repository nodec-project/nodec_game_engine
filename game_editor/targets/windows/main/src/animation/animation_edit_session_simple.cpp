#include "animation_edit_session.hpp"
#include "curve_edit_history.hpp"

#include <nodec_scene/components/hierarchy.hpp>
#include <nodec_scene/components/name.hpp>
#include <nodec_animation/components/animator.hpp>
#include <nodec_animation/systems/animator_system.hpp>
#include <nodec/logging/logging.hpp>

#include <sstream>
#include <algorithm>

namespace nodec_game_editor {
namespace animation {

using namespace nodec_animation::resources;

AnimationEditSession::AnimationEditSession(
    const std::string& session_id,
    nodec::entities::Entity entity,
    std::shared_ptr<AnimationClip> clip,
    const std::string& clip_path)
    : history_(std::make_unique<CurveEditHistory>()) {
    
    session_info_.session_id = session_id;
    session_info_.target_entity = entity;
    session_info_.clip_path = clip_path;
    session_info_.created_at = std::chrono::system_clock::now();
    session_info_.last_modified = session_info_.created_at;
    session_info_.is_dirty = false;
    
    original_clip_ = clip;
    
    // Create working copy
    if (clip) {
        working_clip_ = std::make_shared<AnimationClip>(*clip);
        
        // Initialize curve states - simplified version
        // Just track that we have a clip
    } else {
        // Create empty clip for new animation
        working_clip_ = std::make_shared<AnimationClip>();
    }
}

bool AnimationEditSession::update_curve(
    const std::string& property_path,
    const std::vector<Keyframe>& keyframes) {
    
    if (!working_clip_) return false;
    
    // For now, just mark as dirty
    mark_dirty();
    modified_curves_.insert(property_path);
    
    return true;
}

bool AnimationEditSession::add_curve(
    const std::string& property_path,
    const std::string& component_type) {
    
    if (!working_clip_) return false;
    
    // For now, just mark as dirty
    mark_dirty();
    modified_curves_.insert(property_path);
    
    return true;
}

bool AnimationEditSession::remove_curve(const std::string& property_path) {
    if (!working_clip_) return false;
    
    // For now, just mark as dirty
    mark_dirty();
    modified_curves_.erase(property_path);
    
    return true;
}

const AnimationEditSession::CurveState* 
AnimationEditSession::get_curve_state(const std::string& property_path) const {
    auto it = curve_states_.find(property_path);
    return it != curve_states_.end() ? &it->second : nullptr;
}

std::vector<std::string> AnimationEditSession::get_modified_curves() const {
    return std::vector<std::string>(modified_curves_.begin(), modified_curves_.end());
}

bool AnimationEditSession::can_undo() const {
    return history_ && history_->can_undo();
}

bool AnimationEditSession::can_redo() const {
    return history_ && history_->can_redo();
}

bool AnimationEditSession::undo() {
    if (!history_ || !history_->can_undo()) return false;
    
    auto edit = history_->undo();
    if (!edit) return false;
    
    // Apply the undo - simplified
    mark_dirty();
    
    return true;
}

bool AnimationEditSession::redo() {
    if (!history_ || !history_->can_redo()) return false;
    
    auto edit = history_->redo();
    if (!edit) return false;
    
    // Apply the redo - simplified
    mark_dirty();
    
    return true;
}

void AnimationEditSession::clear_history() {
    if (history_) {
        history_->clear();
    }
}

bool AnimationEditSession::save(const std::string& save_path) {
    if (!working_clip_) return false;
    
    std::string path = save_path.empty() ? session_info_.clip_path : save_path;
    if (path.empty()) return false;
    
    // TODO: Actually save to file system
    // For now, just update the original clip
    original_clip_ = std::make_shared<AnimationClip>(*working_clip_);
    
    // Clear dirty state
    session_info_.is_dirty = false;
    modified_curves_.clear();
    
    clear_history();
    
    return true;
}

bool AnimationEditSession::revert() {
    if (!original_clip_) return false;
    
    // Restore working copy from original
    working_clip_ = std::make_shared<AnimationClip>(*original_clip_);
    
    // Reset states
    session_info_.is_dirty = false;
    modified_curves_.clear();
    
    clear_history();
    
    return true;
}

void AnimationEditSession::apply_to_preview(nodec_world::World* world) {
    if (!world || !working_clip_) return;
    
    // Simplified preview application
    // The actual implementation would apply the animation
}

float AnimationEditSession::get_duration() const {
    if (!working_clip_) return 0.0f;
    
    // Simplified duration calculation
    return 1.0f; // Default duration
}

std::string AnimationEditSession::build_entity_path(
    nodec::entities::Entity entity, 
    nodec::entities::Entity root) const {
    // Simplified path building
    return "";
}

AnimationEditSession::ParsedPropertyPath 
AnimationEditSession::parse_property_path(const std::string& path) const {
    ParsedPropertyPath result;
    
    // Property path format: "entity/path/ComponentType/property.name"
    // or just "ComponentType/property.name" for root entity
    
    size_t last_slash = path.rfind('/');
    if (last_slash == std::string::npos) {
        // Just a property name, invalid
        return result;
    }
    
    result.property_name = path.substr(last_slash + 1);
    
    std::string remaining = path.substr(0, last_slash);
    size_t component_slash = remaining.rfind('/');
    
    if (component_slash == std::string::npos) {
        // No entity path, just component
        result.component_type = remaining;
    } else {
        result.entity_path = remaining.substr(0, component_slash);
        result.component_type = remaining.substr(component_slash + 1);
    }
    
    return result;
}

void AnimationEditSession::mark_dirty() {
    session_info_.is_dirty = true;
    session_info_.last_modified = std::chrono::system_clock::now();
}

void AnimationEditSession::update_curve_state(const std::string& property_path) {
    // Simplified state update
    auto it = curve_states_.find(property_path);
    if (it != curve_states_.end()) {
        it->second.is_modified = true;
    }
}

void AnimationEditSession::push_to_history(
    const std::string& property_path,
    const AnimationCurve& before) {
    
    if (!history_) return;
    
    // Simplified history push
    AnimationCurve after;
    history_->add_edit(property_path, before, after);
}

// Helper methods are implemented inline above

} // namespace animation
} // namespace nodec_game_editor