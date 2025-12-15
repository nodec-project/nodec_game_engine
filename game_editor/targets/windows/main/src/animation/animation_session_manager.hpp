#ifndef NODEC_GAME_EDITOR__ANIMATION__ANIMATION_SESSION_MANAGER_HPP_
#define NODEC_GAME_EDITOR__ANIMATION__ANIMATION_SESSION_MANAGER_HPP_

#include "animation_edit_session.hpp"

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <random>

#include <nodec_world/world.hpp>
#include <nodec/resource_management/resource_registry.hpp>

namespace nodec_game_editor {
namespace animation {

/**
 * @brief Manages multiple animation editing sessions
 * 
 * This singleton class maintains all active animation editing sessions
 * and provides thread-safe access to them.
 */
class AnimationSessionManager {
public:
    /**
     * @brief Get the singleton instance
     */
    static AnimationSessionManager& instance();
    
    /**
     * @brief Create a new animation editing session
     * @param entity Target entity with Animator component
     * @param clip_path Resource path to animation clip (optional for new clips)
     * @param resource_registry Registry to load animation clips from
     * @return Session ID or empty string on failure
     */
    std::string create_session(
        nodec::entities::Entity entity,
        const std::string& clip_path,
        nodec::resource_management::ResourceRegistry& resource_registry
    );
    
    /**
     * @brief Get an existing session
     * @param session_id Session identifier
     * @return Session pointer or nullptr if not found
     */
    std::shared_ptr<AnimationEditSession> get_session(const std::string& session_id);
    
    /**
     * @brief Delete a session
     * @param session_id Session identifier
     * @return true if session was deleted, false if not found
     */
    bool delete_session(const std::string& session_id);
    
    /**
     * @brief Get all active session IDs
     */
    std::vector<std::string> get_all_session_ids() const;
    
    /**
     * @brief Check if a session exists
     */
    bool has_session(const std::string& session_id) const;
    
    /**
     * @brief Clean up expired sessions
     * 
     * Removes sessions that have been inactive for too long
     * @param max_age_minutes Maximum age in minutes before a session is considered expired
     * @return Number of sessions removed
     */
    size_t cleanup_expired_sessions(size_t max_age_minutes = 30);
    
    /**
     * @brief Get total number of active sessions
     */
    size_t get_session_count() const;
    
    /**
     * @brief Update all sessions (called from main loop)
     * @param world Current world for preview updates
     * @param delta_time Time since last update
     */
    void update_all_sessions(nodec_world::World* world, float delta_time);
    
private:
    AnimationSessionManager() = default;
    ~AnimationSessionManager() = default;
    
    // Disable copy and move
    AnimationSessionManager(const AnimationSessionManager&) = delete;
    AnimationSessionManager& operator=(const AnimationSessionManager&) = delete;
    AnimationSessionManager(AnimationSessionManager&&) = delete;
    AnimationSessionManager& operator=(AnimationSessionManager&&) = delete;
    
    /**
     * @brief Generate a unique session ID
     */
    std::string generate_session_id();
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<AnimationEditSession>> sessions_;
    
    // For generating unique IDs
    std::random_device rd_;
    std::mt19937 gen_{rd_()};
    std::uniform_int_distribution<> dis_{0, 15};
};

} // namespace animation
} // namespace nodec_game_editor

#endif // NODEC_GAME_EDITOR__ANIMATION__ANIMATION_SESSION_MANAGER_HPP_