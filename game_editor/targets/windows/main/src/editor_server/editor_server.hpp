#ifndef NODEC_GAME_EDITOR__EDITOR_SERVER_HPP_
#define NODEC_GAME_EDITOR__EDITOR_SERVER_HPP_

#include <memory>
#include <string>

#include <nodec_world/world.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>
#include <nodec/resource_management/resource_registry.hpp>

class EditorServer {
public:
    EditorServer(nodec_world::World* world,
                 nodec_scene_serialization::SceneSerialization* scene_serialization,
                 nodec::resource_management::ResourceRegistry* resource_registry);

    ~EditorServer();

    // Non-copyable, non-movable
    EditorServer(const EditorServer&) = delete;
    EditorServer& operator=(const EditorServer&) = delete;
    EditorServer(EditorServer&&) = delete;
    EditorServer& operator=(EditorServer&&) = delete;

    // Called from Editor::update() to process async requests
    void process_pending_requests();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif
