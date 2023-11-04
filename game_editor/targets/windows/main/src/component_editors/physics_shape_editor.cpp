#include "physics_shape_editor.hpp"

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec/math/gfx.hpp>
#include <nodec_scene_editor/components/selected.hpp>

namespace component_editors {
void PhysicsShapeEditor::on_inspector_gui(nodec_physics::components::PhysicsShape &shape, const nodec_scene_editor::InspectorGuiContext &context) {
    using namespace nodec_physics::components;

    {
        int current = static_cast<int>(shape.shape_type);
        ImGui::Combo("Shape Type", &current, "Box\0Sphere\0Capsule");
        shape.shape_type = static_cast<PhysicsShape::ShapeType>(current);
    }

    switch (shape.shape_type) {
    case PhysicsShape::ShapeType::Box:
        ImGui::DragFloat3("Size", shape.size.v, 0.005f);
        break;

    case PhysicsShape::ShapeType::Sphere:
        ImGui::DragFloat("Radius", &shape.radius, 0.005f);
        break;

    case PhysicsShape::ShapeType::Capsule:
        ImGui::DragFloat("Radius", &shape.radius, 0.005f);
        ImGui::DragFloat("Height", &shape.height, 0.005f);
        break;

    default:
        ImGui::Text("Unsupported shape.");
        break;
    }
}
void PhysicsShapeEditor::on_scene_gui(nodec_scene_editor::SceneGizmo &gizmo,
                                      const nodec_scene_editor::SceneGuiContext &context) {
    auto &scene_registry = context.registry;
    using namespace nodec;
    using namespace nodec_scene_editor::components;
    using namespace nodec_physics::components;
    scene_registry.view<Selected, PhysicsShape>().each(
        [&](nodec_scene::SceneEntity entity, Selected &, PhysicsShape &shape) {
            using namespace nodec_scene::components;
            using namespace nodec;

            auto &local_to_world = scene_registry.get_component<LocalToWorld>(entity);
            Vector3f world_position, world_scale;
            Quaternionf world_rotation;
            math::gfx::decompose_trs(local_to_world.value, world_position, world_rotation, world_scale);

            const auto scale = std::max({world_scale.x, world_scale.y, world_scale.z});

            switch (shape.shape_type) {
            case PhysicsShape::ShapeType::Box: {
                const auto size = shape.size * world_scale;
                gizmo.draw_wire_cube(world_position, size, world_rotation);
            } break;

            case PhysicsShape::ShapeType::Sphere:
                gizmo.draw_wire_sphere(world_position, shape.radius * scale);
                break;

            case PhysicsShape::ShapeType::Capsule: {
                const auto radius = shape.radius * scale;
                const auto height = shape.height * scale;
                const auto half_extent_up = math::gfx::rotate(Vector3f(0.0f, height / 2.0f, 0.0f), world_rotation);
                
                gizmo.draw_wire_sphere(world_position + half_extent_up, radius);
                gizmo.draw_wire_sphere(world_position - half_extent_up, radius);
            } break;

            default:
                break;
            }
        });
}
} // namespace component_editors