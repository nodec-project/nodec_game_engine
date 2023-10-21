#ifndef COMPONENT_EDITORS__PHYSICS_SHAPE_EDITOR_HPP_
#define COMPONENT_EDITORS__PHYSICS_SHAPE_EDITOR_HPP_

#include <nodec_physics/components/physics_shape.hpp>
#include <nodec_scene_editor/component_editor.hpp>

namespace component_editors {

class PhysicsShapeEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_physics::components::PhysicsShape> {
public:
    void on_inspector_gui(nodec_physics::components::PhysicsShape &,
                          const nodec_scene_editor::InspectorGuiContext &) override;

    void on_scene_gui(nodec_scene_editor::SceneGizmo &,
                      const nodec_scene_editor::SceneGuiContext &) override;
};
} // namespace component_editors

#endif