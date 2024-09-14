#ifndef NODEC_GAME_EDITOR__COMPONENT_EDITORS__COLLISION_FILTER_EDITOR_HPP_
#define NODEC_GAME_EDITOR__COMPONENT_EDITORS__COLLISION_FILTER_EDITOR_HPP_

#include <imgui.h>
#include <nodec/formatter.hpp>
#include <nodec_scene_editor/component_editor.hpp>

#include <nodec_physics/components/collision_filter.hpp>

namespace component_editors {

class CollisionFilterEditor
    : public nodec_scene_editor::BasicComponentEditor<nodec_physics::components::CollisionFilter> {
public:
    void select_bits_uint32(const char *label, std::uint32_t &value) {
        ImGui::PushID(label);

        ImGui::Text(label);
        ImGui::SameLine();

        ImGui::Text("%#X", value);
        ImGui::SameLine();

        if (ImGui::Button("Edit")) {
            ImGui::OpenPopup("edit-popup");
        }

        if (ImGui::BeginPopup("edit-popup")) {
            for (int i = 0; i < 32; ++i) {
                if (i & 0x07) {
                    ImGui::SameLine();
                } else {
                    ImGui::Text("%02d", i);
                    ImGui::SameLine();
                }
                bool is_selected = (value & (1 << i)) != 0;
                ImGui::PushID(i);
                if (ImGui::Checkbox("##", &is_selected)) {
                    if (is_selected) {
                        value |= (1 << i);
                    } else {
                        value &= ~(1 << i);
                    }
                }
                ImGui::PopID();
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    void on_inspector_gui(nodec_physics::components::CollisionFilter &filter,
                          const nodec_scene_editor::InspectorGuiContext &context) override {
        select_bits_uint32("Group", filter.group);
        select_bits_uint32("Mask", filter.mask);
    };
};

} // namespace component_editors

#endif