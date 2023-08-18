#ifndef NODEC_SCENE_EDITOR__ENTITY_INSPECTOR_WINDOW_HPP_
#define NODEC_SCENE_EDITOR__ENTITY_INSPECTOR_WINDOW_HPP_

#include <type_traits>
#include <unordered_map>
#include <vector>

#include <imessentials/window.hpp>
#include <imgui.h>
#include <nodec/type_info.hpp>
#include <nodec/vector2.hpp>
#include <nodec_scene/scene_entity.hpp>
#include <nodec_scene/scene_registry.hpp>
#include <nodec_scene_editor/component_registry.hpp>

class EntityInspectorWindow final : public imessentials::BaseWindow {
    using ChangeTargetSignal = nodec::signals::Signal<void(nodec_scene::SceneEntity)>;

public:
    static void help_marker(const char *desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

public:
    EntityInspectorWindow(nodec_scene::SceneRegistry *entity_registry,
                          nodec_scene_editor::ComponentRegistry *component_registry,
                          nodec_scene::SceneEntity init_target_entity,
                          ChangeTargetSignal::SignalInterface change_target_signal)
        : BaseWindow("Entity Inspector", nodec::Vector2f(300, 500)),
          entity_registry_(entity_registry),
          component_registry_(component_registry),
          target_entity_(init_target_entity),
          change_target_signal_connection_(change_target_signal.connect([&](auto entity) { inspect(entity); })) {
    }

    ~EntityInspectorWindow() {}

    void on_gui() override {
        using namespace nodec;

        if (!entity_registry_->is_valid(target_entity_)) {
            return;
        }

        struct InspectionInfo {
            int num_of_total_components{0};
            int num_of_visited_components{0};
        };

        InspectionInfo inspection_info;

        entity_registry_->visit(target_entity_, [&](const nodec::type_info &type_info, void *component) {
            ++inspection_info.num_of_total_components;

            auto *handler = component_registry_->get_handler(type_info);
            if (!handler) return;

            ++inspection_info.num_of_visited_components;

            auto opened = ImGui::CollapsingHeader(handler->component_name().c_str());

            // this context item is bound with last element (CollapsingHeader).
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove")) {
                    handler->remove_component(*entity_registry_, target_entity_);
                }

                ImGui::EndPopup();
            }

            if (opened) {
                handler->editor().on_inspector_gui_opaque(component, {target_entity_, *entity_registry_});
            }
        });

        ImGui::Separator();

        if (ImGui::Button("Add component")) {
            ImGui::OpenPopup("add-component-popup");
        }

        if (ImGui::BeginPopup("add-component-popup")) {
            filter_.Draw("Filter");
            if (ImGui::BeginListBox("##component-list")) {
                for (auto &pair : *component_registry_) {
                    auto &handler = pair.second;

                    const char *component_name = handler->component_name().c_str();

                    if (filter_.IsActive() && !filter_.PassFilter(component_name)) continue;

                    if (ImGui::Selectable(component_name)) {
                        handler->emplace_component(*entity_registry_, target_entity_);
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndListBox();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        ImGui::Text(static_cast<std::string>(Formatter() << "Visited " << inspection_info.num_of_visited_components << " / "
                                                         << inspection_info.num_of_total_components << " components.")
                        .c_str());
        if (inspection_info.num_of_visited_components < inspection_info.num_of_total_components) {
            ImGui::SameLine();
            help_marker(static_cast<std::string>(Formatter() << "To inspect a component, it must be registered through EntityInspectorWindow::ComponentRegistry.").c_str());
        }
    }

private:
    void inspect(const nodec_scene::SceneEntity &entity) {
        target_entity_ = entity;
    }

private:
    nodec_scene::SceneRegistry *entity_registry_{nullptr};
    nodec_scene_editor::ComponentRegistry *component_registry_{nullptr};
    nodec_scene::SceneEntity target_entity_{nodec::entities::null_entity};
    nodec::signals::Connection change_target_signal_connection_;
    ImGuiTextFilter filter_;
};

#endif