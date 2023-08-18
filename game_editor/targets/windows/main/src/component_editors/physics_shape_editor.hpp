#ifndef COMPONENT_EDITORS__PHYSICS_SHAPE_EDITOR_HPP_
#define COMPONENT_EDITORS__PHYSICS_SHAPE_EDITOR_HPP_

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_physics/components/physics_shape.hpp>

namespace component_editors {

class PhysicsShapeEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_physics::components::PhysicsShape> {
public:
    void on_inspector_gui(nodec_physics::components::PhysicsShape &shape,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        using namespace nodec_physics::components;

        {
            int current = static_cast<int>(shape.shape_type);
            ImGui::Combo("Shape Type", &current, "Box\0Sphere");
            shape.shape_type = static_cast<PhysicsShape::ShapeType>(current);
        }

        switch (shape.shape_type) {
        case PhysicsShape::ShapeType::Box:
            ImGui::DragFloat3("Size", shape.size.v);

            break;
        case PhysicsShape::ShapeType::Sphere:
            ImGui::DragFloat("Radius", &shape.radius);

            break;
        default:
            ImGui::Text("Unsupported shape.");
            break;
        }
    }
};
} // namespace component_editors

#endif