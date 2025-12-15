#ifndef NODEC_GAME_EDITOR__ANIMATION__ANIMATION_PREVIEW_CONTROLLER_HPP_
#define NODEC_GAME_EDITOR__ANIMATION__ANIMATION_PREVIEW_CONTROLLER_HPP_

#include "animation_edit_session.hpp"

#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include <chrono>

namespace nodec_game_editor {
namespace animation {

/**
 * @brief Controls animation preview playback for editing sessions
 * 
 * Manages playback state, time updates, and property value streaming
 * for real-time preview of animation edits.
 */
class AnimationPreviewController {
public:
    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };
    
    struct PropertyUpdate {
        std::string property_path;
        float old_value;
        float new_value;
        std::chrono::system_clock::time_point timestamp;
    };
    
    using PropertyUpdateCallback = std::function<void(const std::vector<PropertyUpdate>&)>;
    using TimeUpdateCallback = std::function<void(float time, PlaybackState state)>;

public:
    AnimationPreviewController();
    ~AnimationPreviewController() = default;
    
    /**
     * @brief Set the session to control
     */
    void set_session(std::shared_ptr<AnimationEditSession> session);
    
    /**
     * @brief Get current session
     */
    std::shared_ptr<AnimationEditSession> get_session() const { return session_.lock(); }
    
    /**
     * @brief Update preview (called from main loop)
     * @param delta_time Time elapsed since last update
     */
    void update(float delta_time);
    
    // Playback control
    void play();
    void pause();
    void stop();
    void seek(float time);
    
    // Playback state
    bool is_playing() const { return state_ == PlaybackState::Playing; }
    PlaybackState get_state() const { return state_; }
    float get_current_time() const { return current_time_; }
    float get_playback_speed() const { return playback_speed_; }
    void set_playback_speed(float speed) { playback_speed_ = speed; }
    
    // Loop control
    bool is_looping() const { return loop_enabled_; }
    void set_looping(bool enabled) { loop_enabled_ = enabled; }
    
    // Callbacks for property and time updates
    void on_property_update(PropertyUpdateCallback callback);
    void on_time_update(TimeUpdateCallback callback);
    
    /**
     * @brief Force refresh of all property values
     * 
     * Useful when curves are edited to immediately see changes
     */
    void refresh_preview();
    
    /**
     * @brief Get current property values at the current time
     * @return Map of property path to current value
     */
    std::unordered_map<std::string, float> get_current_property_values() const;
    
private:
    /**
     * @brief Apply animation at current time and detect changes
     */
    void apply_and_broadcast();
    
    /**
     * @brief Compute differential updates
     */
    std::vector<PropertyUpdate> compute_property_diff(
        const std::unordered_map<std::string, float>& new_values
    );
    
    /**
     * @brief Broadcast time update to callbacks
     */
    void broadcast_time_update();
    
    /**
     * @brief Broadcast property updates to callbacks
     */
    void broadcast_property_updates(const std::vector<PropertyUpdate>& updates);

private:
    // Session being controlled
    std::weak_ptr<AnimationEditSession> session_;
    
    // Playback state
    PlaybackState state_ = PlaybackState::Stopped;
    float current_time_ = 0.0f;
    float playback_speed_ = 1.0f;
    bool loop_enabled_ = true;
    
    // Previous property values for differential updates
    std::unordered_map<std::string, float> previous_values_;
    
    // Callbacks
    std::vector<PropertyUpdateCallback> property_callbacks_;
    std::vector<TimeUpdateCallback> time_callbacks_;
    
    // Performance tracking
    std::chrono::steady_clock::time_point last_update_time_;
    float accumulated_time_ = 0.0f;
};

} // namespace animation
} // namespace nodec_game_editor

#endif // NODEC_GAME_EDITOR__ANIMATION__ANIMATION_PREVIEW_CONTROLLER_HPP_