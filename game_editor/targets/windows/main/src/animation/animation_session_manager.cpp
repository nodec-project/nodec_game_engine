#include "animation_session_manager.hpp"

#include <nodec_animation/resources/animation_clip.hpp>
#include <nodec_animation/components/animator.hpp>
#include <nodec/logging/logging.hpp>

#include <sstream>
#include <iomanip>
#include <chrono>

namespace nodec_game_editor {
namespace animation {

using namespace nodec_animation::resources;

AnimationSessionManager& AnimationSessionManager::instance() {
    static AnimationSessionManager instance;
    return instance;
}

std::string AnimationSessionManager::create_session(
    nodec::entities::Entity entity,
    const std::string& clip_path,
    nodec::resource_management::ResourceRegistry& resource_registry) {
    
    if (!entity) {
        nodec::logging::error(__FILE__, __LINE__) 
            << "Cannot create session for null entity";
        return "";
    }
    
    // Load animation clip if path provided
    std::shared_ptr<AnimationClip> clip;
    if (!clip_path.empty()) {
        try {
            clip = resource_registry.get_resource_direct<AnimationClip>(clip_path);
            if (!clip) {
                nodec::logging::warn(__FILE__, __LINE__)
                    << "Animation clip not found: " << clip_path;
                // Continue with null clip for new animation
            }
        } catch (const std::exception& e) {
            nodec::logging::error(__FILE__, __LINE__)
                << "Failed to load animation clip: " << e.what();
            // Continue with null clip
        }
    }
    
    // Generate unique session ID
    std::string session_id = generate_session_id();
    
    // Create the session
    auto session = std::make_shared<AnimationEditSession>(
        session_id,
        entity,
        clip,
        clip_path
    );
    
    // Store session
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[session_id] = session;
    }
    
    nodec::logging::info(__FILE__, __LINE__)
        << "Created animation session: " << session_id
        << " for entity: " << entity
        << " with clip: " << (clip_path.empty() ? "<new>" : clip_path);
    
    return session_id;
}

std::shared_ptr<AnimationEditSession> 
AnimationSessionManager::get_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return it->second;
    }
    
    return nullptr;
}

bool AnimationSessionManager::delete_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        // Check if session has unsaved changes
        if (it->second->is_dirty()) {
            nodec::logging::warn(__FILE__, __LINE__)
                << "Deleting session with unsaved changes: " << session_id;
        }
        
        sessions_.erase(it);
        
        nodec::logging::info(__FILE__, __LINE__)
            << "Deleted animation session: " << session_id;
        
        return true;
    }
    
    return false;
}

std::vector<std::string> AnimationSessionManager::get_all_session_ids() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> ids;
    ids.reserve(sessions_.size());
    
    for (const auto& [id, session] : sessions_) {
        ids.push_back(id);
    }
    
    return ids;
}

bool AnimationSessionManager::has_session(const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.find(session_id) != sessions_.end();
}

size_t AnimationSessionManager::cleanup_expired_sessions(size_t max_age_minutes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto max_age = std::chrono::minutes(max_age_minutes);
    
    std::vector<std::string> to_remove;
    
    for (const auto& [id, session] : sessions_) {
        auto age = now - session->get_info().last_modified;
        if (age > max_age) {
            // Don't remove dirty sessions
            if (!session->is_dirty()) {
                to_remove.push_back(id);
            }
        }
    }
    
    for (const auto& id : to_remove) {
        sessions_.erase(id);
        nodec::logging::info(__FILE__, __LINE__)
            << "Cleaned up expired session: " << id;
    }
    
    return to_remove.size();
}

size_t AnimationSessionManager::get_session_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

void AnimationSessionManager::update_all_sessions(
    nodec_world::World* world, 
    float delta_time) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [id, session] : sessions_) {
        if (session->is_preview_enabled()) {
            // Update preview time if playing
            float current_time = session->get_current_time();
            current_time += delta_time;
            
            // Loop at duration
            float duration = session->get_duration();
            if (duration > 0 && current_time > duration) {
                current_time = std::fmod(current_time, duration);
            }
            
            session->set_current_time(current_time);
            
            // Apply animation to preview
            session->apply_to_preview(world);
        }
    }
}

std::string AnimationSessionManager::generate_session_id() {
    // Generate UUID-like string
    std::stringstream ss;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    // Use timestamp as prefix
    ss << std::hex << time_t << "-";
    
    // Add random suffix
    static const char* hex_chars = "0123456789abcdef";
    for (int i = 0; i < 16; ++i) {
        ss << hex_chars[dis_(gen_)];
        if (i == 3 || i == 7 || i == 11) {
            ss << "-";
        }
    }
    
    return ss.str();
}

} // namespace animation
} // namespace nodec_game_editor