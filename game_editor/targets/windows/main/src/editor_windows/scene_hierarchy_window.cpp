#include "scene_hierarchy_window.hpp"

#include <imgui_internal.h>

#include <nodec_scene_editor/components/selected.hpp>
#include <nodec_scene_serialization/components/prefab.hpp>
#include <nodec_scene_serialization/entity_builder.hpp>
#include <nodec_scene_serialization/entity_serializer.hpp>

void SceneHierarchyWindow::on_gui() {
    using namespace nodec_scene::components;
    using namespace nodec::entities;
    using namespace nodec_scene;
    using namespace nodec_scene_serialization;

    if (ImGui::BeginPopupContextWindow("window-context-menu")) {
        if (ImGui::MenuItem("New Entity")) {
            auto entity = scene_.create_entity("New Entity");
        }
        if (ImGui::MenuItem("Paste", nullptr, false, static_cast<bool>(copied_entity_))) {
            auto entt = scene_.registry().create_entity();
            EntityBuilder(serialization_).build(copied_entity_.get(), entt, scene_);
            scene_.registry().emplace_component<Hierarchy>(entt); // Append it in root.
        }
        ImGui::EndPopup();
    }

    // Make the copy of the roots.
    // Iterator may be invalidated by adding or deleting elements while iterating.

    std::vector<SceneEntity> roots;

    auto root = scene_.hierarchy_system().root_hierarchy().first;
    while (root != null_entity) {
        roots.emplace_back(root);
        root = scene_.registry().get_component<Hierarchy>(root).next;
    }

    for (auto &root : roots) {
        show_entity_node(root);
    }
}

void SceneHierarchyWindow::show_entity_node(const nodec_scene::SceneEntity entity) {
    using namespace nodec;
    using namespace nodec::entities;
    using namespace nodec_scene::components;
    using namespace nodec_scene;
    using namespace nodec_scene_serialization;
    using namespace nodec_scene_serialization::components;
    using namespace nodec_scene_editor::components;

    auto &scene_registry = scene_.registry();

    bool node_open = false;

    Prefab *prefab{nullptr};

    {
        struct DragDropPayloadData {
            SceneEntity entity{null_entity};
            bool is_in_multi_selection{false};
        };

        auto &hierarchy = scene_registry.get_component<Hierarchy>(entity);
        auto name = scene_registry.try_get_component<Name>(entity);
        prefab = scene_registry.try_get_component<Prefab>(entity);
        auto selected = scene_registry.try_get_component<Selected>(entity);

        const auto node_cursor_position = ImGui::GetCursorPos();

        std::string label = nodec::Formatter() << "\"" << (name ? name->value : "") << "\" {entity: 0x" << std::hex << entity << "}";

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

        flags |= hierarchy.child_count > 0 ? 0x00 : ImGuiTreeNodeFlags_Leaf;
        flags |= (selected ? ImGuiTreeNodeFlags_Selected : 0x00);

        auto &style = ImGui::GetStyle();

        node_open = ImGui::TreeNodeEx(label.c_str(), flags);
        const auto node_size = ImGui::GetItemRectSize();

        if (prefab) {
            ImDrawList *draw_list = ImGui::GetWindowDrawList();

            ImVec2 marker_min(ImGui::GetItemRectMin());
            ImVec2 marker_max(ImGui::GetItemRectMin().x + 3, ImGui::GetItemRectMin().y + ImGui::GetItemRectSize().y);
            draw_list->AddRectFilled(marker_min, marker_max, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_HeaderActive]));
        }

        // * <https://github.com/ocornut/imgui/issues/2597>
        if (ImGui::BeginDragDropSource()) {
            DragDropPayloadData data;
            data.entity = entity;
            data.is_in_multi_selection = selected != nullptr;
            ImGui::SetDragDropPayload("DND_SCENE_ENTITY", &data, sizeof(DragDropPayloadData));

            ImGui::Text("Move Entity");

            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (auto *payload = ImGui::AcceptDragDropPayload("DND_SCENE_ENTITY")) {
                IM_ASSERT(payload->DataSize == sizeof(DragDropPayloadData));
                auto *data = reinterpret_cast<DragDropPayloadData *>(payload->Data);
                if (data->is_in_multi_selection) {
                    scene_registry.view<Selected>().each([&](const SceneEntity &selected_entity, Selected &) {
                        try {
                            scene_.hierarchy_system().append_child(entity, selected_entity);
                        } catch (std::runtime_error &error) {
                            // The exception occurs when the parent of entity is set into itself.
                            logger_->error(__FILE__, __LINE__) << error.what();
                        }
                    });
                } else {
                    try {
                        scene_.hierarchy_system().append_child(entity, data->entity);
                    } catch (std::runtime_error &error) {
                        // The exception occurs when the parent of entity is set into itself.
                        logger_->error(__FILE__, __LINE__) << error.what();
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        const auto drag_drop_active = ImGui::GetCurrentContext()->DragDropActive;

        if (!drag_drop_active && ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            // The left mouse button was released while hovering over the item

            if (ImGui::GetIO().KeyCtrl && selected) {
                scene_registry.remove_component<Selected>(entity);
                selected = nullptr;
            } else {
                select(entity, ImGui::GetIO().KeyCtrl);
            }
        }

        // Use last item id as popup id
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Copy")) {
                copied_entity_ = EntitySerializer(serialization_).serialize(entity, scene_);
            }

            if (ImGui::MenuItem("Paste", nullptr, false, static_cast<bool>(copied_entity_))) {
                auto entt = scene_registry.create_entity();
                EntityBuilder(serialization_).build(copied_entity_.get(), entt, scene_);
                scene_.hierarchy_system().append_child(entity, entt);
            }

            if (ImGui::MenuItem("Delete")) {
                scene_registry.destroy_entity(entity);
            }

            if (ImGui::MenuItem("Move to root")) {
                auto parent = scene_registry.get_component<Hierarchy>(entity).parent;
                if (parent != nodec::entities::null_entity) {
                    scene_.hierarchy_system().remove_child(parent, entity);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("New Entity")) {
                auto child = scene_.create_entity("New Entity");
                scene_.hierarchy_system().append_child(entity, child);
            }

            ImGui::EndPopup();
        }

        if (drag_drop_active) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            const auto next_cursor_pos = ImGui::GetCursorPos();

            ImGui::SetCursorPos(ImVec2(node_cursor_position.x, node_cursor_position.y - 2));

            ImGui::InvisibleButton("##top", ImVec2(node_size.x, 2));
            if (ImGui::BeginDragDropTarget()) {
                if (auto *payload = ImGui::AcceptDragDropPayload("DND_SCENE_ENTITY")) {
                    IM_ASSERT(payload->DataSize == sizeof(DragDropPayloadData));
                    auto *data = reinterpret_cast<DragDropPayloadData *>(payload->Data);
                    if (data->is_in_multi_selection) {
                        scene_registry.view<Selected>().each([&](const SceneEntity &selected_entity, Selected &) {
                            try {
                                scene_.hierarchy_system().insert_before(selected_entity, entity);
                            } catch (std::runtime_error &error) {
                                // The exception occurs when the parent of entity is set into itself.
                                logger_->error(__FILE__, __LINE__) << error.what();
                            }
                        });
                    } else {
                        try {
                            scene_.hierarchy_system().insert_before(data->entity, entity);
                        } catch (std::runtime_error &error) {
                            // The exception occurs when the parent of entity is set into itself.
                            logger_->error(__FILE__, __LINE__) << error.what();
                        }
                    }
                }
            }

            ImGui::SetCursorPos(ImVec2(node_cursor_position.x, node_cursor_position.y + node_size.y));
            ImGui::InvisibleButton("##bottom", ImVec2(node_size.x, 2));
            if (ImGui::BeginDragDropTarget()) {
                if (auto *payload = ImGui::AcceptDragDropPayload("DND_SCENE_ENTITY")) {
                    IM_ASSERT(payload->DataSize == sizeof(DragDropPayloadData));
                    auto *data = reinterpret_cast<DragDropPayloadData *>(payload->Data);
                    if (data->is_in_multi_selection) {
                        scene_registry.view<Selected>().each([&](const SceneEntity &selected_entity, Selected &) {
                            try {
                                scene_.hierarchy_system().insert_after(selected_entity, entity);
                            } catch (std::runtime_error &error) {
                                // The exception occurs when the parent of entity is set into itself.
                                logger_->error(__FILE__, __LINE__) << error.what();
                            }
                        });
                    } else {
                        try {
                            scene_.hierarchy_system().insert_after(data->entity, entity);
                        } catch (std::runtime_error &error) {
                            // The exception occurs when the parent of entity is set into itself.
                            logger_->error(__FILE__, __LINE__) << error.what();
                        }
                    }
                }
            }

            ImGui::SetCursorPos(next_cursor_pos);
            ImGui::PopStyleVar();
        }
    }

    if (node_open) {
        if (scene_registry.is_valid(entity)) {
            auto &hierarchy = scene_registry.get_component<Hierarchy>(entity);

            auto child = hierarchy.first;
            while (child != null_entity) {
                // We first save next entity.
                // During show_entity_node(), the child entity may be destroyed.
                auto next = scene_registry.get_component<Hierarchy>(child).next;

                show_entity_node(child);

                child = next;
            }
        }

        ImGui::TreePop();
    }
}

void SceneHierarchyWindow::select(nodec_scene::SceneEntity entity, bool additional) {
    using namespace nodec_scene_editor::components;
    using namespace nodec::entities;

    auto &scene_registry = scene_.registry();

    if (!additional) {
        auto view = scene_registry.view<Selected>();
        scene_registry.remove_component<Selected>(view.begin(), view.end());
    }
    if (!scene_registry.is_valid(entity)) return;
    scene_registry.emplace_component<Selected>(entity);
}