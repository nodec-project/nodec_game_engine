#ifndef COMPONENT_EDITORS__MESH_RENDERER_EDITOR_HPP_
#define COMPONENT_EDITORS__MESH_RENDERER_EDITOR_HPP_

#include <imessentials/list.hpp>
#include <imgui.h>
#include <nodec/gfx/gfx.hpp>
#include <nodec_resources/resources.hpp>
#include <nodec_scene/components/local_to_world.hpp>
#include <nodec_scene_editor/component_editor.hpp>
#include <nodec_scene_editor/components/selected.hpp>

#include <nodec_rendering/components/mesh_renderer.hpp>

#include <rendering/mesh_backend.hpp>

#include "../editor_gui.hpp"

namespace component_editors {

class MeshRendererEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::MeshRenderer> {
public:
    MeshRendererEditor(EditorGui &gui, nodec_resources::Resources &resources)
        : gui_(gui), resources_(resources) {}

    void on_inspector_gui(nodec_rendering::components::MeshRenderer &renderer,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        using namespace nodec;
        using namespace nodec_rendering::resources;
        using namespace nodec_rendering;
        {
            imessentials::list_edit(
                "Meshes", renderer.meshes,
                [&](int index, auto &mesh) {
                    mesh = gui_.resource_field("", mesh);
                },
                [&]() {
                    auto empty = resources_.registry().get_resource_direct<Mesh>("");
                    renderer.meshes.emplace_back(empty);
                });
        }
        {
            imessentials::list_edit(
                "Materials", renderer.materials,
                [&](int index, auto &material) {
                    material = gui_.resource_field("", material);
                },
                [&]() {
                    auto empty = resources_.registry().get_resource_direct<Material>("");
                    renderer.materials.emplace_back(empty);
                });
        }
    }

    void on_scene_gui(nodec_scene_editor::SceneGizmo &gizmo,
                      const nodec_scene_editor::SceneGuiContext &context) override {
        using namespace nodec_rendering::components;
        using namespace nodec_scene_editor::components;
        using namespace nodec_scene::components;

        static const nodec::Vector4f wire_color{0.4f, 0.6f, 1.0f, 1.0f};

        context.registry.view<Selected, MeshRenderer, LocalToWorld>().each(
            [&](auto entity, const Selected &, const MeshRenderer &renderer, const LocalToWorld &ltw) {
                for (const auto &mesh : renderer.meshes) {
                    if (!mesh) continue;
                    auto *mesh_backend = static_cast<MeshBackend *>(mesh.get());

                    const auto &bounds = mesh_backend->bounds;

                    nodec::Vector3f scale, translation;
                    nodec::Quaternionf rotation;
                    nodec::gfx::decompose_trs(ltw.value, translation, rotation, scale);

                    auto world_center = translation + nodec::gfx::rotate(bounds.center * scale, rotation);
                    auto world_size = bounds.extents * 2.0f * scale;

                    gizmo.draw_wire_cube(world_center, world_size, rotation, wire_color);
                }
            });
    }

private:
    EditorGui &gui_;
    nodec_resources::Resources &resources_;
};
} // namespace component_editors

#endif