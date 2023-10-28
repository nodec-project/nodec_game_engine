#include "physics_shape_editor.hpp"

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec_scene_editor/components/selected.hpp>

namespace component_editors {
void PhysicsShapeEditor::on_inspector_gui(nodec_physics::components::PhysicsShape &shape, const nodec_scene_editor::InspectorGuiContext &context) {
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
void PhysicsShapeEditor::on_scene_gui(nodec_scene_editor::SceneGizmo &gizmo,
                                      const nodec_scene_editor::SceneGuiContext &context) {
    auto &scene_registry = context.registry;
    using namespace nodec_scene_editor::components;
    using namespace nodec_physics::components;
    scene_registry.view<Selected, PhysicsShape>().each(
        [&](nodec_scene::SceneEntity entity, Selected &, PhysicsShape &shape) {
            switch (shape.shape_type) {
            case PhysicsShape::ShapeType::Box:
                gizmo.draw_wire_cube(entity, shape.size);
                break;

            case PhysicsShape::ShapeType::Sphere:
                gizmo.draw_wire_sphere(entity, shape.radius);
                break;

            default:
                break;
            }
        });
}
} // namespace component_editors