#ifndef COMPONENT_EDITORS__NON_VISIBLE_EDITOR_HPP_
#define COMPONENT_EDITORS__NON_VISIBLE_EDITOR_HPP_

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_rendering/components/non_visible.hpp>

namespace component_editors {

class NonVisibleEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_rendering::components::NonVisible> {
public:
    void on_inspector_gui(nodec_rendering::components::NonVisible &non_visible,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        ImGui::Checkbox("Self", &non_visible.self);
    }
};
} // namespace component_editors

#endif