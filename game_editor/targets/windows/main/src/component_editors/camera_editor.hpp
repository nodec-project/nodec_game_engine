#ifndef COMPONENT_EDITORS__CAMERA_EDITOR_HPP_
#define COMPONENT_EDITORS__CAMERA_EDITOR_HPP_

#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_rendering/components/camera.hpp>

namespace component_editors {

class CameraEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::Camera> {
public:
    void on_inspector_gui(nodec_rendering::components::Camera &camera,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        using namespace nodec_rendering::components;

        ImGui::DragFloat("Near Clip Plane", &camera.near_clip_plane);
        ImGui::DragFloat("Far Clip Plane", &camera.far_clip_plane);

        {
            int current = static_cast<int>(camera.projection);
            ImGui::Combo("Projection", &current, "Perspective\0Orthographic");
            camera.projection = static_cast<Camera::Projection>(current);
        }

        switch (camera.projection) {
        case Camera::Projection::Perspective:
            ImGui::DragFloat("Fov Angle", &camera.fov_angle);
            break;
        case Camera::Projection::Orthographic:
            ImGui::DragFloat("Ortho Width", &camera.ortho_width);
        default: break;
        }
    }
};
} // namespace component_editors

#endif