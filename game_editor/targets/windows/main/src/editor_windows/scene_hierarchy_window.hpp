#ifndef SCENE_HIERARCHY_WINDOW_HPP_
#define SCENE_HIERARCHY_WINDOW_HPP_

#include <imessentials/window.hpp>
#include <nodec_scene/scene.hpp>
#include <nodec_scene_serialization/entity_builder.hpp>
#include <nodec_scene_serialization/entity_serializer.hpp>

#include <imgui.h>

class SceneHierarchyWindow final : public imessentials::BaseWindow {
public:
    SceneHierarchyWindow(nodec_scene::Scene *scene,
                         nodec_scene_serialization::SceneSerialization &serialization)
        : BaseWindow("Scene Hierarchy", nodec::Vector2f(200, 500)),
          serialization_(serialization),
          scene_(scene) {
    }

    void on_gui() override;

    decltype(auto) selected_entity_changed() {
        return selected_entity_changed_.signal_interface();
    }

private:
    void show_entity_node(const nodec_scene::SceneEntity entity);

    void select(nodec_scene::SceneEntity entity) {
        if (entity == selected_entity_) return;

        selected_entity_ = entity;
        selected_entity_changed_(selected_entity_);
    }

private:
    nodec_scene_serialization::SceneSerialization &serialization_;

    nodec_scene::Scene *scene_{nullptr};
    nodec_scene::SceneEntity selected_entity_{nodec::entities::null_entity};

    nodec::signals::Signal<void(nodec_scene::SceneEntity selected)> selected_entity_changed_;

    std::unique_ptr<nodec_scene_serialization::SerializableEntity> copied_entity_;
};

#endif