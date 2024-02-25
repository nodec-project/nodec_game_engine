#ifndef COMPONENT_EDITORS__IMAGE_RENDERER_EDITOR_HPP_
#define COMPONENT_EDITORS__IMAGE_RENDERER_EDITOR_HPP_

#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

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

private:
    EditorGui &gui_;
};
} // namespace component_editors

#endif