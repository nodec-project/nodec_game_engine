#include "animation_preview_controller.hpp"

#include <nodec/logging/logging.hpp>
#include <algorithm>
#include <cmath>

namespace nodec_game_editor {
namespace animation {

AnimationPreviewController::AnimationPreviewController()
    : last_update_time_(std::chrono::steady_clock::now()) {
}

void AnimationPreviewController::set_session(std::shared_ptr<AnimationEditSession> session) {
    session_ = session;
    
    // Reset state when changing sessions
    stop();
    previous_values_.clear();
    
    if (session) {
        // Initialize with current property values
        refresh_preview();
    }
}

void AnimationPreviewController::update(float delta_time) {
    auto session = session_.lock();
    if (!session) return;
    
    if (state_ == PlaybackState::Playing) {
        // Update time with speed multiplier
        accumulated_time_ += delta_time * playback_speed_;
        
        // Apply in fixed timesteps for consistency
        const float fixed_timestep = 1.0f / 60.0f; // 60 FPS
        
        while (accumulated_time_ >= fixed_timestep) {
            current_time_ += fixed_timestep;
            accumulated_time_ -= fixed_timestep;
            
            // Handle looping
            float duration = session->get_duration();
            if (duration > 0) {
                if (loop_enabled_) {
                    if (current_time_ > duration) {
                        current_time_ = std::fmod(current_time_, duration);
                    }
                } else {
                    if (current_time_ > duration) {
                        current_time_ = duration;
                        pause(); // Stop at end if not looping
                    }
                }
            }
        }
        
        // Update session time
        session->set_current_time(current_time_);
        
        // Apply animation and broadcast updates
        apply_and_broadcast();
    }
}

void AnimationPreviewController::play() {
    auto session = session_.lock();
    if (!session) return;
    
    if (state_ != PlaybackState::Playing) {
        state_ = PlaybackState::Playing;
        session->set_preview_enabled(true);
        
        // Reset accumulator
        accumulated_time_ = 0.0f;
        last_update_time_ = std::chrono::steady_clock::now();
        
        broadcast_time_update();
        
        nodec::logging::info(__FILE__, __LINE__)
            << "Started animation preview playback";
    }
}

void AnimationPreviewController::pause() {
    if (state_ == PlaybackState::Playing) {
        state_ = PlaybackState::Paused;
        
        broadcast_time_update();
        
        nodec::logging::info(__FILE__, __LINE__)
            << "Paused animation preview at " << current_time_ << "s";
    }
}

void AnimationPreviewController::stop() {
    auto session = session_.lock();
    
    state_ = PlaybackState::Stopped;
    current_time_ = 0.0f;
    accumulated_time_ = 0.0f;
    
    if (session) {
        session->set_current_time(0.0f);
        session->set_preview_enabled(false);
    }
    
    broadcast_time_update();
    refresh_preview(); // Reset to initial state
    
    nodec::logging::info(__FILE__, __LINE__)
        << "Stopped animation preview";
}

void AnimationPreviewController::seek(float time) {
    auto session = session_.lock();
    if (!session) return;
    
    // Clamp time to valid range
    float duration = session->get_duration();
    if (duration > 0) {
        time = std::clamp(time, 0.0f, duration);
    } else {
        time = std::max(0.0f, time);
    }
    
    current_time_ = time;
    session->set_current_time(time);
    
    apply_and_broadcast();
    broadcast_time_update();
}

void AnimationPreviewController::on_property_update(PropertyUpdateCallback callback) {
    if (callback) {
        property_callbacks_.push_back(callback);
    }
}

void AnimationPreviewController::on_time_update(TimeUpdateCallback callback) {
    if (callback) {
        time_callbacks_.push_back(callback);
    }
}

void AnimationPreviewController::refresh_preview() {
    auto session = session_.lock();
    if (!session) return;
    
    // Force update even if not playing
    apply_and_broadcast();
}

std::unordered_map<std::string, float> 
AnimationPreviewController::get_current_property_values() const {
    auto session = session_.lock();
    if (!session) {
        return {};
    }
    
    std::unordered_map<std::string, float> values;
    
    // Get all curve states from session
    auto states = session->get_all_curve_states();
    
    // Evaluate each curve at current time
    for (const auto& [path, state] : states) {
        auto keyframes = state.current_curve.keyframes();
        if (!keyframes.empty()) {
            // Simple linear interpolation for now
            // TODO: Use proper curve evaluation
            float value = 0.0f;
            
            // Find surrounding keyframes
            for (size_t i = 0; i < keyframes.size(); ++i) {
                if (keyframes[i].time >= current_time_) {
                    if (i == 0) {
                        value = keyframes[i].value;
                    } else {
                        // Interpolate between keyframes
                        const auto& k0 = keyframes[i - 1];
                        const auto& k1 = keyframes[i];
                        float t = (current_time_ - k0.time) / (k1.time - k0.time);
                        value = k0.value + t * (k1.value - k0.value);
                    }
                    break;
                } else if (i == keyframes.size() - 1) {
                    // Past last keyframe
                    value = keyframes[i].value;
                }
            }
            
            values[path] = value;
        }
    }
    
    return values;
}

void AnimationPreviewController::apply_and_broadcast() {
    auto session = session_.lock();
    if (!session) return;
    
    // Get current property values
    auto current_values = get_current_property_values();
    
    // Compute differential updates
    auto updates = compute_property_diff(current_values);
    
    // Update previous values
    previous_values_ = current_values;
    
    // Broadcast updates if any
    if (!updates.empty()) {
        broadcast_property_updates(updates);
    }
    
    // Apply to preview in engine
    // Note: This would be done through the World parameter
    // session->apply_to_preview(world);
}

std::vector<AnimationPreviewController::PropertyUpdate>
AnimationPreviewController::compute_property_diff(
    const std::unordered_map<std::string, float>& new_values) {
    
    std::vector<PropertyUpdate> updates;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& [path, new_value] : new_values) {
        float old_value = 0.0f;
        
        auto it = previous_values_.find(path);
        if (it != previous_values_.end()) {
            old_value = it->second;
        }
        
        // Only report changes above threshold
        if (std::abs(new_value - old_value) > 0.0001f) {
            PropertyUpdate update;
            update.property_path = path;
            update.old_value = old_value;
            update.new_value = new_value;
            update.timestamp = now;
            
            updates.push_back(update);
        }
    }
    
    // Check for removed properties
    for (const auto& [path, old_value] : previous_values_) {
        if (new_values.find(path) == new_values.end()) {
            PropertyUpdate update;
            update.property_path = path;
            update.old_value = old_value;
            update.new_value = 0.0f; // Default value for removed
            update.timestamp = now;
            
            updates.push_back(update);
        }
    }
    
    return updates;
}

void AnimationPreviewController::broadcast_time_update() {
    for (const auto& callback : time_callbacks_) {
        callback(current_time_, state_);
    }
}

void AnimationPreviewController::broadcast_property_updates(
    const std::vector<PropertyUpdate>& updates) {
    
    for (const auto& callback : property_callbacks_) {
        callback(updates);
    }
}

} // namespace animation
} // namespace nodec_game_editor