#ifndef COMPONENT_EDITORS__TEXT_RENDERER_EDITOR_HPP_
#define COMPONENT_EDITORS__TEXT_RENDERER_EDITOR_HPP_

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_rendering/components/text_renderer.hpp>

#include "../editor_gui.hpp"

namespace component_editors {

class TextRendererEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::TextRenderer> {
public:
    TextRendererEditor(EditorGui &gui)
        : gui_(gui) {}

    void on_inspector_gui(nodec_rendering::components::TextRenderer &renderer,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        auto &buffer = imessentials::get_text_buffer(1024, renderer.text);
        ImGui::InputTextMultiline("Text", buffer.data(), buffer.size());
        renderer.text = buffer.data();
        renderer.font = gui_.resource_field("Font", renderer.font);
        renderer.material = gui_.resource_field("Material", renderer.material);

        ImGui::ColorEdit4("Color", renderer.color.v, ImGuiColorEditFlags_Float);

        ImGui::DragInt("Pixel Size", &renderer.pixel_size);
        ImGui::DragInt("Pixels Per Unit", &renderer.pixels_per_unit);
    }

private:
    EditorGui &gui_;
};
} // namespace component_editors

#endif