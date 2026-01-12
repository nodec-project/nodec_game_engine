#ifndef COMPONENT_EDITORS__IMAGE_RENDERER_EDITOR_HPP_
#define COMPONENT_EDITORS__IMAGE_RENDERER_EDITOR_HPP_

#include <limits>

#include <imgui.h>
#include <nodec/gfx/gfx.hpp>
#include <nodec_scene/components/local_to_world.hpp>
#include <nodec_scene_editor/component_editor.hpp>
#include <nodec_scene_editor/components/selected.hpp>

#include <nodec_rendering/components/image_renderer.hpp>

#include "../editor_gui.hpp"

namespace component_editors {

class ImageRendererEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::ImageRenderer> {
public:
    ImageRendererEditor(EditorGui &gui)
        : gui_(gui) {}

    void on_inspector_gui(nodec_rendering::components::ImageRenderer &renderer,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        renderer.image = gui_.resource_field("Image", renderer.image);
        renderer.material = gui_.resource_field("Material", renderer.material);
        ImGui::ColorEdit4("Color", renderer.color.v, ImGuiColorEditFlags_Float);
        ImGui::DragInt("Pixels Per Unit", &renderer.pixels_per_unit);
    }

    void on_scene_gui(nodec_scene_editor::SceneGizmo &gizmo,
                      const nodec_scene_editor::SceneGuiContext &context) override {
        using namespace nodec_rendering::components;
        using namespace nodec_scene_editor::components;
        using namespace nodec_scene::components;

        static const nodec::Vector4f wire_color{0.4f, 0.6f, 1.0f, 1.0f};

        context.registry.view<Selected, ImageRenderer, LocalToWorld>().each(
            [&](auto entity, const Selected &, const ImageRenderer &renderer, const LocalToWorld &ltw) {
                if (!renderer.image) return;

                const float ppu = renderer.pixels_per_unit + std::numeric_limits<float>::epsilon();
                const float width = renderer.image->width() / ppu;
                const float height = renderer.image->height() / ppu;

                nodec::Vector3f scale, translation;
                nodec::Quaternionf rotation;
                nodec::gfx::decompose_trs(ltw.value, translation, rotation, scale);

                auto world_size = nodec::Vector3f(width, height, 0.01f) * scale;

                gizmo.draw_wire_cube(translation, world_size, rotation, wire_color);
            });
    }

private:
    EditorGui &gui_;
};
} // namespace component_editors

#endif