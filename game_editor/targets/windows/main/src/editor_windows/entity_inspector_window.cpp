#include "entity_inspector_window.hpp"

#include <nodec_scene_editor/components/selected.hpp>

namespace {

void help_marker(const char *desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

} // namespace

EntityInspectorWindow::EntityInspectorWindow(nodec_scene::SceneRegistry &entity_registry,
                                             nodec_scene_editor::ComponentRegistry &component_registry,
                                             nodec_scene_serialization::SceneSerialization &scene_serialization)
    : BaseWindow("Entity Inspector", nodec::Vector2f(300, 500)),
      entity_registry_(entity_registry),
      component_registry_(component_registry),
      scene_serialization_(scene_serialization) {
}

void EntityInspectorWindow::on_gui() {
    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene_serialization;

    SceneEntity selected_entity{nodec::entities::null_entity};
    {
        auto view = entity_registry_.view<nodec_scene_editor::components::Selected>();
        if (view.begin() == view.end()) return;
        selected_entity = *view.begin();
    }

    struct InspectionInfo {
        int num_of_total_components{0};
        int num_of_visited_components{0};
    };

    InspectionInfo inspection_info;

    entity_registry_.visit(selected_entity, [&](const nodec::type_info &type_info, void *component) {
        ++inspection_info.num_of_total_components;

        auto *handler = component_registry_.get_handler(type_info);
        if (!handler) return;

        ++inspection_info.num_of_visited_components;

        auto opened = ImGui::CollapsingHeader(handler->component_name().c_str());

        // this context item is bound with last element (CollapsingHeader).
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Copy")) {
                copied_component_type_info_ = &type_info;
                copied_component_ = scene_serialization_.make_serializable_component(type_info, component);
            }

            if (ImGui::MenuItem("Remove")) {
                handler->remove_component(entity_registry_, selected_entity);
            }

            ImGui::Separator();
            {
                bool enabled = true;
                if (!copied_component_type_info_ || !copied_component_) {
                    enabled = false;
                }
                if (copied_component_type_info_ != &type_info) {
                    enabled = false;
                }
                if (ImGui::MenuItem("Paste Properties", nullptr, false, enabled)) {
                    scene_serialization_.assign_component(
                        copied_component_.get(), type_info, component);
                }
            }

            ImGui::EndPopup();
        }

        if (opened) {
            handler->editor().on_inspector_gui_opaque(component, {selected_entity, entity_registry_});
        }
    });

    ImGui::Separator();

    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("add-component-popup");
    }
    if (ImGui::BeginPopupContextItem()) {
        {
            bool enabled = true;
            if (!copied_component_type_info_ || !copied_component_) {
                enabled = false;
            }

            if (ImGui::MenuItem("Paste", nullptr, false, enabled)) {
                scene_serialization_.emplace_or_replace_component(
                    copied_component_.get(), selected_entity, entity_registry_);
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("add-component-popup")) {
        filter_.Draw("Filter");
        if (ImGui::BeginListBox("##component-list")) {
            for (auto &pair : component_registry_) {
                auto &handler = pair.second;

                const char *component_name = handler->component_name().c_str();

                if (filter_.IsActive() && !filter_.PassFilter(component_name)) continue;

                if (ImGui::Selectable(component_name)) {
                    handler->emplace_component(entity_registry_, selected_entity);
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
