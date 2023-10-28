#ifndef COMPONENT_EDITORS__NAME_EDITOR_HPP_
#define COMPONENT_EDITORS__NAME_EDITOR_HPP_

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec_scene/components/name.hpp>
#include <nodec_scene_editor/component_editor.hpp>

namespace component_editors {

class NameEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_scene::components::Name> {
public:
    void on_inspector_gui(nodec_scene::components::Name &name, const nodec_scene_editor::InspectorGuiContext &context) override {
        auto &buffer = imessentials::get_text_buffer(1024, name.value);

        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputText("##name", buffer.data(), buffer.size(), input_text_flags)) {
            // on type enter
        }
        name.value = buffer.data();
    }
};
} // namespace component_editors

#endif