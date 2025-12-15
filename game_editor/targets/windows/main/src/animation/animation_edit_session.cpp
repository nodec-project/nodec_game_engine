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
        
        // Initialize curve states from existing clip
        // Walk through the clip hierarchy to collect all curves
        std::function<void(const AnimatedEntity&, const std::string&)> collect_curves;
        collect_curves = [&](const AnimatedEntity& entity, const std::string& entity_path) {
            // Process components
            for (const auto& [type_info, component] : entity.components) {
                std::string component_name = "Component_" + std::to_string(type_info.seq_index());
                
                for (const auto& [prop_name, prop] : component.properties) {
                    std::string full_path = entity_path.empty() ?
                        component_name + "/" + prop_name :
                        entity_path + "/" + component_name + "/" + prop_name;
                    
                    CurveState state;
                    state.property_path = full_path;
                    state.original_curve = prop.curve;
                    state.current_curve = prop.curve;
                    state.is_modified = false;
                    
                    curve_states_[full_path] = state;
                }
            }
            
            // Process children
            for (const auto& [child_name, child_entity] : entity.children) {
                std::string child_path = entity_path.empty() ? 
                    child_name : 
                    entity_path + "/" + child_name;
                collect_curves(child_entity, child_path);
            }
        };
        
        collect_curves(clip->root_entity(), "");
    } else {
        // Create empty clip for new animation
        working_clip_ = std::make_shared<AnimationClip>();
    }
}

bool AnimationEditSession::update_curve(
    const std::string& property_path,
    const std::vector<Keyframe>& keyframes) {
    
    if (!working_clip_) return false;
    
    auto* curve = find_curve_by_path(working_clip_->root_entity(), property_path);
    if (!curve) {
        // Curve doesn't exist, try to add it
        return add_curve(property_path);
    }
    
    // Store previous state for undo
    push_to_history(property_path, *curve);
    
    // Update curve
    curve->set_keyframes(std::vector<Keyframe>(keyframes));
    
    // Update state
    update_curve_state(property_path);
    mark_dirty();
    
    return true;
}

bool AnimationEditSession::add_curve(
    const std::string& property_path,
    const std::string& component_type) {
    
    if (!working_clip_) return false;
    
    auto parsed = parse_property_path(property_path);
    if (parsed.property_name.empty()) return false;
    
    // Navigate to or create the entity path
    AnimatedEntity* target = &working_clip_->root_entity();
    if (!parsed.entity_path.empty()) {
        std::istringstream path_stream(parsed.entity_path);
        std::string segment;
        while (std::getline(path_stream, segment, '/')) {
            target = &target->children[segment];
        }
    }
    
    // Add the curve to the component
    // For now, use a placeholder type_info
    nodec::type_info type_info(0); // TODO: Get actual component type
    AnimationCurve new_curve;
    
    // Add default keyframes
    new_curve.add_keyframe({0.0f, 0.0f});
    new_curve.add_keyframe({1.0f, 1.0f});
    
    target->components[type_info].properties[parsed.property_name].curve = new_curve;
    
    // Update state
    CurveState state;
    state.property_path = property_path;
    state.original_curve = AnimationCurve(); // Empty original
    state.current_curve = new_curve;
    state.is_modified = true;
    
    curve_states_[property_path] = state;
    modified_curves_.insert(property_path);
    mark_dirty();
    
    return true;
}

bool AnimationEditSession::remove_curve(const std::string& property_path) {
    if (!working_clip_) return false;
    
    auto parsed = parse_property_path(property_path);
    if (parsed.property_name.empty()) return false;
    
    // Navigate to the entity
    AnimatedEntity* target = &working_clip_->root_entity();
    if (!parsed.entity_path.empty()) {
        std::istringstream path_stream(parsed.entity_path);
        std::string segment;
        while (std::getline(path_stream, segment, '/')) {
            auto it = target->children.find(segment);
            if (it == target->children.end()) return false;
            target = &it->second;
        }
    }
    
    // Find and remove the property
    for (auto& [type_info, component] : target->components) {
        auto it = component.properties.find(parsed.property_name);
        if (it != component.properties.end()) {
            // Store for undo
            push_to_history(property_path, it->second.curve);
            
            component.properties.erase(it);
            
            // Update state
            curve_states_.erase(property_path);
            modified_curves_.insert(property_path);
            mark_dirty();
            
            return true;
        }
    }
    
    return false;
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
    
    // Apply the undo
    auto* curve = find_curve_by_path(working_clip_->root_entity(), edit->property_path);
    if (curve) {
        *curve = edit->before_curve;
        update_curve_state(edit->property_path);
    }
    
    return true;
}

bool AnimationEditSession::redo() {
    if (!history_ || !history_->can_redo()) return false;
    
    auto edit = history_->redo();
    if (!edit) return false;
    
    // Apply the redo
    auto* curve = find_curve_by_path(working_clip_->root_entity(), edit->property_path);
    if (curve) {
        *curve = edit->after_curve;
        update_curve_state(edit->property_path);
    }
    
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
    
    // Update curve states
    for (auto& [path, state] : curve_states_) {
        state.original_curve = state.current_curve;
        state.is_modified = false;
    }
    
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
    
    // Reset curve states
    for (auto& [path, state] : curve_states_) {
        state.current_curve = state.original_curve;
        state.is_modified = false;
    }
    
    clear_history();
    
    return true;
}

void AnimationEditSession::apply_to_preview(nodec_world::World* world) {
    if (!world || !working_clip_) return;
    
    auto& registry = world->scene().registry();
    auto* animator = registry.try_get_component<nodec_animation::components::Animator>(
        session_info_.target_entity
    );
    
    if (!animator) return;
    
    // Temporarily set the working clip
    auto original = animator->clip;
    animator->clip = working_clip_;
    
    // Apply animation at current time
    // This would typically be done by the AnimatorSystem
    // For now, we'll just update the clip reference
    
    // Restore original clip
    // Note: In actual implementation, we might want to keep the working clip
    // active during the entire session
}

float AnimationEditSession::get_duration() const {
    if (!working_clip_) return 0.0f;
    
    float max_time = 0.0f;
    
    std::function<void(const AnimatedEntity&)> find_max_time;
    find_max_time = [&](const AnimatedEntity& entity) {
        for (const auto& [type, component] : entity.components) {
            for (const auto& [prop_name, prop] : component.properties) {
                if (!prop.curve.keyframes().empty()) {
                    max_time = std::max(max_time, prop.curve.keyframes().back().time);
                }
            }
        }
        
        for (const auto& [child_name, child_entity] : entity.children) {
            find_max_time(child_entity);
        }
    };
    
    find_max_time(working_clip_->root_entity());
    
    return max_time;
}

AnimationCurve* AnimationEditSession::find_curve_by_path(
    AnimatedEntity& entity,
    const std::string& property_path,
    const std::string& current_path) {
    
    // Parse the property path to extract entity path and property name
    auto parsed = parse_property_path(property_path);
    
    // Check if we're at the right entity level
    if (current_path == parsed.entity_path) {
        // Look for the property in components
        for (auto& [type_info, component] : entity.components) {
            auto it = component.properties.find(parsed.property_name);
            if (it != component.properties.end()) {
                return &it->second.curve;
            }
        }
    }
    
    // Search in children
    for (auto& [child_name, child_entity] : entity.children) {
        std::string child_path = current_path.empty() ? 
            child_name : 
            current_path + "/" + child_name;
        
        auto* result = find_curve_by_path(child_entity, property_path, child_path);
        if (result) return result;
    }
    
    return nullptr;
}

const AnimationCurve* AnimationEditSession::find_curve_by_path(
    const AnimatedEntity& entity,
    const std::string& property_path,
    const std::string& current_path) const {
    
    // Const version - delegates to non-const version
    return const_cast<AnimationEditSession*>(this)->find_curve_by_path(
        const_cast<AnimatedEntity&>(entity), property_path, current_path
    );
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
    auto it = curve_states_.find(property_path);
    if (it == curve_states_.end()) return;
    
    auto* current = find_curve_by_path(working_clip_->root_entity(), property_path);
    if (!current) return;
    
    it->second.current_curve = *current;
    
    // Check if modified from original
    bool is_modified = false;
    const auto& orig = it->second.original_curve.keyframes();
    const auto& curr = current->keyframes();
    
    if (orig.size() != curr.size()) {
        is_modified = true;
    } else {
        for (size_t i = 0; i < orig.size(); ++i) {
            if (std::abs(orig[i].time - curr[i].time) > 0.0001f ||
                std::abs(orig[i].value - curr[i].value) > 0.0001f) {
                is_modified = true;
                break;
            }
        }
    }
    
    it->second.is_modified = is_modified;
    
    if (is_modified) {
        modified_curves_.insert(property_path);
    } else {
        modified_curves_.erase(property_path);
    }
}

void AnimationEditSession::push_to_history(
    const std::string& property_path,
    const AnimationCurve& before) {
    
    if (!history_) return;
    
    // Get the after state
    auto* after_curve = find_curve_by_path(working_clip_->root_entity(), property_path);
    if (!after_curve) return;
    
    history_->add_edit(property_path, before, *after_curve);
}

} // namespace animation
} // namespace nodec_game_editor