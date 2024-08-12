#ifndef NODEC_GAME_EDITOR__SCENE_GIZMO_IMPL_HPP_
#define NODEC_GAME_EDITOR__SCENE_GIZMO_IMPL_HPP_

#include <nodec/gfx/gfx.hpp>

#include <nodec_rendering/components/mesh_renderer.hpp>
#include <nodec_resources/resources.hpp>
#include <nodec_scene/components/local_to_world.hpp>
#include <nodec_scene/scene_registry.hpp>
#include <nodec_scene_editor/scene_gizmo.hpp>

#include "components/gizmo_wire.hpp"

class SceneGizmoImpl : public nodec_scene_editor::SceneGizmo {
public:
    SceneGizmoImpl(nodec_scene::Scene &scene, nodec_resources::Resources &resources)
        : scene_(scene), resources_(resources) {
        using namespace nodec_rendering::resources;
        wire_cube_mesh_ = resources_.registry().get_resource_direct<Mesh>("org.nodec.game-editor/meshes/wire-cube.mesh");
        wire_sphere_mesh_ = resources_.registry().get_resource_direct<Mesh>("org.nodec.game-editor/meshes/wire-sphere.mesh");
    }

    void draw_wire_cube(const nodec::Vector3f &center, const nodec::Vector3f &size,
                        const nodec::Quaternionf &rotation) override {
        using namespace nodec_scene::components;

        auto gizmo_entity = scene_.registry().create_entity();
        auto &gizmo_wire = scene_.registry().emplace_component<components::GizmoWire>(gizmo_entity).first;
        gizmo_wire.mesh = wire_cube_mesh_;
        auto &local_to_world = scene_.registry().emplace_component<LocalToWorld>(gizmo_entity).first;
        local_to_world.value = nodec::gfx::trs(center, rotation, size);
        local_to_world.dirty = true;
    }

    void draw_wire_sphere(const nodec::Vector3f &center, float radius) override {
        using namespace nodec_scene::components;

        auto gizmo_entity = scene_.registry().create_entity();
        auto &gizmo_wire = scene_.registry().emplace_component<components::GizmoWire>(gizmo_entity).first;
        gizmo_wire.mesh = wire_sphere_mesh_;
        auto &local_to_world = scene_.registry().emplace_component<LocalToWorld>(gizmo_entity).first;
        local_to_world.value = nodec::gfx::trs(center, nodec::Quaternionf::identity, nodec::Vector3f::ones * radius);
        local_to_world.dirty = true;
    }

private:
    nodec_scene::Scene &scene_;
    nodec_resources::Resources &resources_;

    std::shared_ptr<nodec_rendering::resources::Mesh> wire_cube_mesh_;
    std::shared_ptr<nodec_rendering::resources::Mesh> wire_sphere_mesh_;
};

#endif