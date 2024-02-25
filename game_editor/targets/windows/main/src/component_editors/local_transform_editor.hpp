#ifndef COMPONENT_EDITORS__LOCAL_TRANSFORM_EDITOR_HPP_
#define COMPONENT_EDITORS__LOCAL_TRANSFORM_EDITOR_HPP_

#include <imgui.h>
#include <nodec/math/gfx.hpp>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_scene/components/local_transform.hpp>

namespace component_editors {

class LocalTransformEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_scene::components::LocalTransform> {
public:
    void on_inspector_gui(nodec_scene::components::LocalTransform &trfm,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        using namespace nodec;
        {
            ImGui::Text("Position");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::DragFloat3("##Position", trfm.position.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f")) {
                trfm.dirty = true;
            }
        }

        {
            if (!is_rotation_active_) {
                active_euler_angles_ = math::gfx::euler_angles_xyz(trfm.rotation);
            }

            ImGui::Text("Rotation (XYZ Euler)");

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            if (ImGui::DragFloat3("##Rotation(XYZ)", active_euler_angles_.v, 0.1f, -FLT_MAX, +FLT_MAX, "%.3f")) {
                trfm.rotation = math::gfx::euler_angles_xyz(active_euler_angles_);
                trfm.dirty = true;
            }

            is_rotation_active_ = ImGui::IsItemActive();
        }
        {
            ImGui::Text("Rotation (Quaternion)");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::BeginDisabled();
            ImGui::DragFloat4("##Rotation(Quaternion)", trfm.rotation.q, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
            ImGui::EndDisabled();
        }

        {
            ImGui::Text("Scale");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::DragFloat3("##Scale", trfm.scale.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f")) {
                trfm.dirty = true;
            }
        }
    }

private:
    bool is_rotation_active_{false};
    nodec::Vector3f active_euler_angles_;
};
} // namespace component_editors

#endif