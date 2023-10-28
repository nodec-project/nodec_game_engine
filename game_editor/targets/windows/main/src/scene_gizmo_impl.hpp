#ifndef NODEC_GAME_EDITOR__SCENE_GIZMO_IMPL_HPP_
#define NODEC_GAME_EDITOR__SCENE_GIZMO_IMPL_HPP_

#include <nodec/math/gfx.hpp>

#include <nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_resources/resources.hpp>
#include <nodec_scene/components/local_to_world.hpp>
#include <nodec_scene/scene_registry.hpp>
#include <nodec_scene_editor/scene_gizmo.hpp>

#include "components/gizmo_wire.hpp"

class SceneGizmoImpl : public nodec_scene_editor::SceneGizmo {
    void emplace_gizmo_wire(const nodec_scene::SceneEntity &entity,
                            std::shared_ptr<nodec_rendering::resources::Mesh> mesh,
                            const nodec::Vector3f &translate,
                            const nodec::Quaternionf &rotation,
                            const nodec::Vector3f &scale) {
        using namespace nodec_scene::components;
        using namespace components;
        auto &scene_registry = scene_.registry();

        auto gizmo_entity = scene_registry.create_entity();
        scene_.hierarchy_system().append_child(entity, gizmo_entity);
        auto &gizmo_wire = scene_registry.emplace_component<GizmoWire>(gizmo_entity).first;
        gizmo_wire.mesh = mesh;

        auto &local_to_world = scene_registry.emplace_component<LocalToWorld>(entity).first;

        auto &gizmo_local_to_world = scene_registry.emplace_component<LocalToWorld>(gizmo_entity).first;
        gizmo_local_to_world.value = local_to_world.value * nodec::math::gfx::trs(translate, rotation, scale);
    }

public:
    SceneGizmoImpl(nodec_scene::Scene &scene, nodec_resources::Resources &resources)
        : scene_(scene), resources_(resources) {
        using namespace nodec_rendering::resources;
        wire_cube_mesh_ = resources_.registry().get_resource_direct<Mesh>("org.nodec.game-editor/meshes/wire-cube.mesh");
        wire_sphere_mesh_ = resources_.registry().get_resource_direct<Mesh>("org.nodec.game-editor/meshes/wire-sphere.mesh");
    }

    void draw_wire_cube(const nodec_scene::SceneEntity &entity,
                        const nodec::Vector3f &size,
                        const nodec::Vector3f &offset,
                        const nodec::Quaternionf &rotation) override {
        emplace_gizmo_wire(entity, wire_cube_mesh_, offset, rotation, size);
    }

    void draw_wire_sphere(const nodec_scene::SceneEntity &entity,
                          float radius,
                          const nodec::Vector3f &offset) override {
        using namespace nodec;
        emplace_gizmo_wire(entity, wire_sphere_mesh_, offset, Quaternionf::identity, Vector3f::ones * radius);
    }

private:
    nodec_scene::Scene &scene_;
    nodec_resources::Resources &resources_;

    std::shared_ptr<nodec_rendering::resources::Mesh> wire_cube_mesh_;
    std::shared_ptr<nodec_rendering::resources::Mesh> wire_sphere_mesh_;
};

#endif