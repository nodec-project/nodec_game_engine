#ifndef COMPONENT_EDITORS__MESH_RENDERER_EDITOR_HPP_
#define COMPONENT_EDITORS__MESH_RENDERER_EDITOR_HPP_

#include <imessentials/list.hpp>
#include <imgui.h>
#include <nodec_resources/resources.hpp>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_rendering/components/mesh_renderer.hpp>

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

private:
    EditorGui &gui_;
    nodec_resources::Resources &resources_;
};
} // namespace component_editors

#endif