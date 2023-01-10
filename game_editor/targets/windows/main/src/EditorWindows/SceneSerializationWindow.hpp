#pragma once

#include "../ResourceExporter.hpp"
#include "../ResourceImporter.hpp"

#include <imessentials/text_buffer.hpp>
#include <imessentials/window.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>

#include <imgui.h>

class SceneSerializationWindow final : public imessentials::BaseWindow {
    using SceneEntity = nodec_scene::SceneEntity;
    using SelectedEntityChangedSignal = nodec::signals::Signal<void(SceneEntity)>;
    using ResourceRegistry = nodec::resource_management::ResourceRegistry;
    using SceneSerialization = nodec_scene_serialization::SceneSerialization;
    using Scene = nodec_scene::Scene;

public:
    SceneSerializationWindow(nodec_scene::Scene *scene,
                             SceneSerialization *scene_serialization,
                             const std::string &resource_path,
                             ResourceRegistry *resource_registry,
                             SceneEntity selected_entity,
                             SelectedEntityChangedSignal::SignalInterface selected_entity_changed_signal)
        : BaseWindow("Scene Serialization", nodec::Vector2f(400, 200)),
          scene_(scene),
          scene_serialization_(scene_serialization),
          resource_path_(resource_path),
          resource_registry_(resource_registry),
          selected_entity_(selected_entity),
          selected_entity_changed_signal_connection_(selected_entity_changed_signal.connect([=](auto entity) { selected_entity_ = entity; })) {}

    void on_gui() override {
        using namespace nodec;
        using namespace nodec::entities;
        using namespace nodec_scene::components;

        // auto import_header_opened = ImGui::CollapsingHeader("Import");

        if (ImGui::BeginTabBar("TabBar")) {
            if (ImGui::BeginTabItem("Import")) {
                {
                    auto &buffer = imessentials::get_text_buffer(1024, import_path_);
                    ImGui::InputText("Source", buffer.data(), buffer.size());
                    import_path_ = buffer.data();
                }

                {
                    int current = static_cast<int>(import_target);
                    ImGui::Combo("Target", &current, "Root\0Selected Entity");
                    import_target = static_cast<Target>(current);
                }

                if (ImGui::Button("Import")) {
                    import_messages.clear();

                    bool success = false;
                    switch (import_target) {
                    case Target::Root: {
                        success = ResourceImporter::ImportSceneGraph(import_path_, null_entity, *resource_registry_, *scene_, *scene_serialization_);
                        break;
                    }
                    case Target::SelectedEntity: {
                        if (scene_->registry().is_valid(selected_entity_)) {
                            success = ResourceImporter::ImportSceneGraph(import_path_, selected_entity_, *resource_registry_, *scene_, *scene_serialization_);
                        }
                        break;
                    }
                    default: break;
                    }

                    if (success) {
                        import_messages.emplace_back(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                                                     Formatter() << "Scene import success: " << import_path_);
                    } else {
                        import_messages.emplace_back(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                                                     Formatter() << "Scene import failed: " << import_path_);
                    }
                }

                print_messages(import_messages);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Export")) {
                {
                    int current = static_cast<int>(export_target);
                    ImGui::Combo("Source", &current, "Root\0Selected Entity");
                    export_target = static_cast<Target>(current);
                }

                {
                    auto &buffer = imessentials::get_text_buffer(1024, export_path_);
                    ImGui::InputText("Target", buffer.data(), buffer.size());
                    export_path_ = buffer.data();
                }

                bool success = false;

                if (ImGui::Button("Export")) {
                    export_messages.clear();

                    std::string dest_path = Formatter() << resource_path_ << "/" << export_path_;

                    switch (export_target) {
                    case Target::Root: {
                        std::vector<SceneEntity> roots;

                        auto root = scene_->hierarchy_system().root_hierarchy().first;
                        while (root != null_entity) {
                            roots.emplace_back(root);
                            root = scene_->registry().get_component<Hierarchy>(root).next;
                        }

                        success = ResourceExporter::ExportSceneGraph(roots, scene_->registry(), *scene_serialization_, *resource_registry_, dest_path);
                        break;
                    }

                    case Target::SelectedEntity: {
                        if (scene_->registry().is_valid(selected_entity_)) {
                            std::vector<SceneEntity> roots{selected_entity_};
                            success = ResourceExporter::ExportSceneGraph(roots, scene_->registry(), *scene_serialization_, *resource_registry_, dest_path);
                        }
                        break;
                    }

                    default: break;
                    }

                    if (success) {
                        export_messages.emplace_back(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                                                     Formatter() << "Scene export success: " << dest_path);
                    } else {
                        export_messages.emplace_back(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                                                     Formatter() << "Scene export failed: " << dest_path);
                    }
                }

                print_messages(export_messages);

                // ImGui::Text("This is the Avocado tab!\nblah blah blah blah blah");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

private:
    struct MessageRecord {
        MessageRecord(const ImVec4 &color, const std::string &message)
            : color(color), message(message) {
        }

        ImVec4 color;
        std::string message;
    };

private:
    static void print_messages(const std::vector<MessageRecord> &messages) {
        for (auto &record : messages) {
            ImGui::PushStyleColor(ImGuiCol_Text, record.color);
            ImGui::TextWrapped(record.message.c_str());
            ImGui::PopStyleColor();
        }
    }

private:
    enum class Target {
        Root,
        SelectedEntity
    };

    std::string import_path_{"level0.scene"};
    std::string export_path_{"level0.scene"};

    std::vector<MessageRecord> export_messages;
    std::vector<MessageRecord> import_messages;

    Target import_target{Target::Root};
    Target export_target{Target::Root};

    std::string resource_path_;
    Scene *scene_;
    ResourceRegistry *resource_registry_;
    SceneSerialization *scene_serialization_;

    SceneEntity selected_entity_;

    nodec::signals::Connection selected_entity_changed_signal_connection_;
};