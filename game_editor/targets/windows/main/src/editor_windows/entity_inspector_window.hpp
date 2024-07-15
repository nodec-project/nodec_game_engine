#ifndef NODEC_SCENE_EDITOR__EDITOR_WINDOWS__ENTITY_INSPECTOR_WINDOW_HPP_
#define NODEC_SCENE_EDITOR__EDITOR_WINDOWS__ENTITY_INSPECTOR_WINDOW_HPP_

#include <imessentials/window.hpp>
#include <imgui.h>
#include <nodec_scene/scene_entity.hpp>
#include <nodec_scene/scene_registry.hpp>
#include <nodec_scene_editor/component_registry.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>

class EntityInspectorWindow final : public imessentials::BaseWindow {
public:
    EntityInspectorWindow(nodec_scene::SceneRegistry &entity_registry,
                          nodec_scene_editor::ComponentRegistry &component_registry,
                          nodec_scene_serialization::SceneSerialization &scene_serialization);

    void on_gui() override;

private:
    nodec_scene::SceneRegistry &entity_registry_;
    nodec_scene_editor::ComponentRegistry &component_registry_;
    nodec_scene_serialization::SceneSerialization &scene_serialization_;
    ImGuiTextFilter filter_;
    std::unique_ptr<nodec_scene_serialization::BaseSerializableComponent>
        copied_component_{nullptr};
    const nodec::type_info* copied_component_type_info_{nullptr};
};

#endif