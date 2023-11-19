#ifndef COMPONENT_EDITORS__DIRECTIONAL_LIGHT_EDITOR_HPP_
#define COMPONENT_EDITORS__DIRECTIONAL_LIGHT_EDITOR_HPP_

#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_rendering/components/directional_light.hpp>

namespace component_editors {

class DirectionalLightEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::DirectionalLight> {
public:
    void on_inspector_gui(nodec_rendering::components::DirectionalLight &light,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        ImGui::ColorEdit4("Color", light.color.v, ImGuiColorEditFlags_Float);
        ImGui::DragFloat("Intensity", &light.intensity, 0.005f, 0.0f, 1.0f);
    }
};
} // namespace component_editors

#endif