#ifndef COMPONENT_EDITORS__POINT_LIGHT_EDITOR_HPP_
#define COMPONENT_EDITORS__POINT_LIGHT_EDITOR_HPP_

#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_rendering/components/point_light.hpp>

namespace component_editors {

class PointLightEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::PointLight> {
public:
    void on_inspector_gui(nodec_rendering::components::PointLight &light,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        ImGui::ColorEdit4("Color", light.color.v, ImGuiColorEditFlags_Float);

        ImGui::DragFloat("Intensity", &light.intensity, 0.005f, 0.0f, 1.0f);

        ImGui::DragFloat("Range", &light.range, 0.005f);
    }
};
} // namespace component_editors

#endif