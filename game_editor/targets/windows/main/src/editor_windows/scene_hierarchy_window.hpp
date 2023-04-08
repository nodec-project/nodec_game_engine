#ifndef SCENE_HIERARCHY_WINDOW_HPP_
#define SCENE_HIERARCHY_WINDOW_HPP_

#include <imessentials/window.hpp>
#include <nodec_scene/scene.hpp>
#include <nodec_scene_serialization/entity_builder.hpp>
#include <nodec_scene_serialization/entity_serializer.hpp>

#include <imgui.h>

class SceneHierarchyWindow final : public imessentials::BaseWindow {
public:
    SceneHierarchyWindow(nodec_scene::Scene *scene,
                         nodec_scene_serialization::SceneSerialization &serialization)
        : BaseWindow("Scene Hierarchy", nodec::Vector2f(200, 500)),
          serialization_(serialization),
          scene_(scene) {
    }

    void on_gui() override {
        using namespace nodec_scene::components;
        using namespace nodec::entities;
        using namespace nodec_scene;
        using namespace nodec_scene_serialization;

        if (!scene_) return;

        if (ImGui::BeginPopupContextWindow("window-contex-menu")) {
            if (ImGui::MenuItem("New Entity")) {
                auto entity = scene_->create_entity("New Entity");
            }
            if (ImGui::MenuItem("Paste", nullptr, false, static_cast<bool>(copied_entity_))) {
                auto entt = scene_->registry().create_entity();
                EntityBuilder(serialization_).build(copied_entity_.get(), entt, *scene_);
                scene_->registry().emplace_component<Hierarchy>(entt); // Append it in root.
            }
            ImGui::EndPopup();
        }

        // Make the copy of the roots.
        // Iterator may be invalidated by adding or deleting elements while iterating.

        std::vector<SceneEntity> roots;

        auto root = scene_->hierarchy_system().root_hierarchy().first;
        while (root != null_entity) {
            roots.emplace_back(root);
            root = scene_->registry().get_component<Hierarchy>(root).next;
        }

        for (auto &root : roots) {
            show_entity_node(root);
        }
    }

    decltype(auto) selected_entity_changed() {
        return selected_entity_changed_.signal_interface();
    }

private:
    void show_entity_node(const nodec_scene::SceneEntity entity) {
        using namespace nodec;
        using namespace nodec::entities;
        using namespace nodec_scene::components;
        using namespace nodec_scene;
        using namespace nodec_scene_serialization;

        bool node_open = false;
        {
            auto &hierarchy = scene_->registry().get_component<Hierarchy>(entity);
            auto name = scene_->registry().try_get_component<Name>(entity);

            std::string label = nodec::Formatter() << "\"" << (name ? name->name : "") << "\" {entity: 0x" << std::hex << entity << "}";

            ImGuiTreeNodeFlags flags = (hierarchy.child_count > 0 ? 0x00 : ImGuiTreeNodeFlags_Leaf)
                                       | (entity == selected_entity_ ? ImGuiTreeNodeFlags_Selected : 0x00)
                                       | ImGuiTreeNodeFlags_OpenOnArrow; // | ImGuiTreeNodeFlags_OpenOnDoubleClick;

            node_open = ImGui::TreeNodeEx(label.c_str(), flags);

            if (ImGui::IsItemClicked()) {
                select(entity);
            }

            // * <https://github.com/ocornut/imgui/issues/2597>
            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("DND_SCENE_ENTITY", &entity, sizeof(SceneEntity));

                ImGui::Text("Move entity");

                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (auto *payload = ImGui::AcceptDragDropPayload("DND_SCENE_ENTITY")) {
                    IM_ASSERT(payload->DataSize == sizeof(SceneEntity));
                    SceneEntity drop_entity = *static_cast<SceneEntity *>(payload->Data);

                    try {
                        scene_->hierarchy_system().append_child(entity, drop_entity);
                    } catch (std::runtime_error &error) {
                        // The exception occurs when the parent of entity is set into itself.
                        logging::ErrorStream(__FILE__, __LINE__) << error.what();
                    }
                }

                ImGui::EndDragDropTarget();
            }
        }

        // Use last item id as popup id
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Copy")) {
                copied_entity_ = EntitySerializer(serialization_).serialize(entity, *scene_);
            }

            if (ImGui::MenuItem("Paste", nullptr, false, static_cast<bool>(copied_entity_))) {
                auto entt = scene_->registry().create_entity();
                EntityBuilder(serialization_).build(copied_entity_.get(), entt, *scene_);
                scene_->hierarchy_system().append_child(entity, entt);
            }

            if (ImGui::MenuItem("Delete")) {
                scene_->registry().destroy_entity(entity);
            }

            if (ImGui::MenuItem("Move to root")) {
                auto parent = scene_->registry().get_component<Hierarchy>(entity).parent;
                if (parent != nodec::entities::null_entity) {
                    scene_->hierarchy_system().remove_child(parent, entity);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("New Entity")) {
                auto child = scene_->create_entity("New Entity");
                scene_->hierarchy_system().append_child(entity, child);
            }

            ImGui::EndPopup();
        }

        if (node_open) {
            if (scene_->registry().is_valid(entity)) {
                auto &hierarchy = scene_->registry().get_component<Hierarchy>(entity);

                auto child = hierarchy.first;
                while (child != null_entity) {
                    // We first save next entity.
                    // During show_entity_node(), the child entity may be destroyed.
                    auto next = scene_->registry().get_component<Hierarchy>(child).next;

                    show_entity_node(child);

                    child = next;
                }
            }

            ImGui::TreePop();
        }
    }

    void select(nodec_scene::SceneEntity entity) {
        if (entity == selected_entity_) return;

        selected_entity_ = entity;
        selected_entity_changed_(selected_entity_);
    }

private:
    nodec_scene_serialization::SceneSerialization &serialization_;

    nodec_scene::Scene *scene_{nullptr};
    nodec_scene::SceneEntity selected_entity_{nodec::entities::null_entity};

    nodec::signals::Signal<void(nodec_scene::SceneEntity selected)> selected_entity_changed_;

    std::unique_ptr<nodec_scene_serialization::SerializableEntity> copied_entity_;
};

#endif