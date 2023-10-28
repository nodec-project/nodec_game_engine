#ifndef COMPONENT_EDITORS__RIGID_BODY_EDITOR_HPP_
#define COMPONENT_EDITORS__RIGID_BODY_EDITOR_HPP_

#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_physics/components/rigid_body.hpp>

namespace component_editors {

class RIgidBodyEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_physics::components::RigidBody> {
public:
    void on_inspector_gui(nodec_physics::components::RigidBody &rigid_body,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        using namespace nodec_physics::components;

        ImGui::DragFloat("Mass", &rigid_body.mass);

        if (ImGui::TreeNode("Constrains")) {
            ImGui::PushID("Freeze Position");
            {
                ImGui::Text("Freeze Position");

                bool flag;
                flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezePositionX);
                ImGui::Checkbox("X", &flag);
                flag ? rigid_body.constraints |= RigidBodyConstraints::FreezePositionX
                     : rigid_body.constraints &= ~RigidBodyConstraints::FreezePositionX;

                ImGui::SameLine();

                flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezePositionY);
                ImGui::Checkbox("Y", &flag);
                flag ? rigid_body.constraints |= RigidBodyConstraints::FreezePositionY
                     : rigid_body.constraints &= ~RigidBodyConstraints::FreezePositionY;

                ImGui::SameLine();

                flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezePositionZ);
                ImGui::Checkbox("Z", &flag);
                flag ? rigid_body.constraints |= RigidBodyConstraints::FreezePositionZ
                     : rigid_body.constraints &= ~RigidBodyConstraints::FreezePositionZ;

                ImGui::PopID();
            }

            ImGui::PushID("Freeze Rotation");
            {
                ImGui::Text("Freeze Rotation");
                bool flag;
                flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezeRotationX);
                ImGui::Checkbox("X", &flag);
                flag ? rigid_body.constraints |= RigidBodyConstraints::FreezeRotationX
                     : rigid_body.constraints &= ~RigidBodyConstraints::FreezeRotationX;

                ImGui::SameLine();

                flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezeRotationY);
                ImGui::Checkbox("Y", &flag);
                flag ? rigid_body.constraints |= RigidBodyConstraints::FreezeRotationY
                     : rigid_body.constraints &= ~RigidBodyConstraints::FreezeRotationY;

                ImGui::SameLine();

                flag = static_cast<bool>(rigid_body.constraints & RigidBodyConstraints::FreezeRotationZ);
                ImGui::Checkbox("Z", &flag);
                flag ? rigid_body.constraints |= RigidBodyConstraints::FreezeRotationZ
                     : rigid_body.constraints &= ~RigidBodyConstraints::FreezeRotationZ;

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Info")) {
            ImGui::DragFloat3("Linear Velocity", rigid_body.linear_velocity.v);
            ImGui::DragFloat3("Angular Velocity", rigid_body.angular_velocity.v);

            ImGui::TreePop();
        }
    }
};
} // namespace component_editors

#endif