#ifndef COMPONENT_EDITORS__RIGID_BODY_EDITOR_HPP_
#define COMPONENT_EDITORS__RIGID_BODY_EDITOR_HPP_

#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_physics/components/rigid_body.hpp>

namespace component_editors {

class RigidBodyEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_physics::components::RigidBody> {
public:
    void on_inspector_gui(nodec_physics::components::RigidBody &rigid_body,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        using namespace nodec_physics::components;

        {
            int current = static_cast<int>(rigid_body.body_type);
            ImGui::Combo("Body Type", &current, "Dynamic\0Kinematic");
            rigid_body.body_type = static_cast<RigidBody::BodyType>(current);
        }

        ImGui::DragFloat("Mass", &rigid_body.mass, 0.005f);

        if (ImGui::TreeNode("Constrains")) {
            ImGui::PushID("Freeze Position");
            auto constraints = rigid_body.constraints;
            {
                ImGui::Text("Freeze Position");

                bool flag;
                flag = static_cast<bool>(constraints & RigidBodyConstraints::FreezePositionX);
                ImGui::Checkbox("X", &flag);
                flag ? constraints |= RigidBodyConstraints::FreezePositionX
                     : constraints &= ~RigidBodyConstraints::FreezePositionX;

                ImGui::SameLine();

                flag = static_cast<bool>(constraints & RigidBodyConstraints::FreezePositionY);
                ImGui::Checkbox("Y", &flag);
                flag ? constraints |= RigidBodyConstraints::FreezePositionY
                     : constraints &= ~RigidBodyConstraints::FreezePositionY;

                ImGui::SameLine();

                flag = static_cast<bool>(constraints & RigidBodyConstraints::FreezePositionZ);
                ImGui::Checkbox("Z", &flag);
                flag ? constraints |= RigidBodyConstraints::FreezePositionZ
                     : constraints &= ~RigidBodyConstraints::FreezePositionZ;

                ImGui::PopID();
            }

            ImGui::PushID("Freeze Rotation");
            {
                ImGui::Text("Freeze Rotation");
                bool flag;
                flag = static_cast<bool>(constraints & RigidBodyConstraints::FreezeRotationX);
                ImGui::Checkbox("X", &flag);
                flag ? constraints |= RigidBodyConstraints::FreezeRotationX
                     : constraints &= ~RigidBodyConstraints::FreezeRotationX;

                ImGui::SameLine();

                flag = static_cast<bool>(constraints & RigidBodyConstraints::FreezeRotationY);
                ImGui::Checkbox("Y", &flag);
                flag ? constraints |= RigidBodyConstraints::FreezeRotationY
                     : constraints &= ~RigidBodyConstraints::FreezeRotationY;

                ImGui::SameLine();

                flag = static_cast<bool>(constraints & RigidBodyConstraints::FreezeRotationZ);
                ImGui::Checkbox("Z", &flag);
                flag ? constraints |= RigidBodyConstraints::FreezeRotationZ
                     : constraints &= ~RigidBodyConstraints::FreezeRotationZ;

                ImGui::PopID();
            }
            if (constraints != rigid_body.constraints) {
                rigid_body.constraints = constraints;
                auto &dirty = context.registry.emplace_component<RigidBodyDirty>(context.entity).first;
                dirty.flags |= RigidBodyDirtyFlag::Constraints;
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