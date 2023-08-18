#ifndef COMPONENT_EDITORS__POST_PROCESSING_EDITOR_HPP_
#define COMPONENT_EDITORS__POST_PROCESSING_EDITOR_HPP_

#include <imessentials/list.hpp>
#include <imgui.h>
#include <nodec_resources/resources.hpp>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_rendering/components/post_processing.hpp>

#include "../editor_gui.hpp"

namespace component_editors {

class PostProcessingEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::PostProcessing> {
public:
    PostProcessingEditor(EditorGui &gui, nodec_resources::Resources &resources)
        : gui_(gui), resources_(resources) {}

    void on_inspector_gui(nodec_rendering::components::PostProcessing &processing,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        using namespace nodec_rendering::components;
        using namespace nodec_rendering::resources;

        imessentials::list_edit(
            "Effects", processing.effects,
            [&](int index, PostProcessing::Effect &effect) {
                ImGui::Checkbox("##enabled", &(effect.enabled));
                ImGui::SameLine();

                auto &material = effect.material;
                material = gui_.resource_field("##material", material);
            },
            [&]() {
                auto material = resources_.registry().get_resource_direct<Material>("");
                processing.effects.push_back({false, material});
            });
    }

private:
    EditorGui &gui_;
    nodec_resources::Resources &resources_;
};
} // namespace component_editors

#endif