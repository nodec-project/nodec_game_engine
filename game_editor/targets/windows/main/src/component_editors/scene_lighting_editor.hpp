#ifndef COMPONENT_EDITORS__SCENE_LIGHTING_EDITOR_HPP_
#define COMPONENT_EDITORS__SCENE_LIGHTING_EDITOR_HPP_

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_rendering/components/scene_lighting.hpp>

#include "../editor_gui.hpp"

namespace component_editors {

class SceneLightingEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::SceneLighting> {
public:
    SceneLightingEditor(EditorGui &gui)
        : gui_(gui) {}

    void on_inspector_gui(nodec_rendering::components::SceneLighting &lighting,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        lighting.skybox = gui_.resource_field("Skybox", lighting.skybox);
        ImGui::ColorEdit4("Ambient Color", lighting.ambient_color.v, ImGuiColorEditFlags_Float);
    }

private:
    EditorGui &gui_;
};
} // namespace component_editors

#endif