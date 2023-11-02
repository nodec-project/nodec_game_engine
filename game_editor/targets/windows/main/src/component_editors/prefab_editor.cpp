#include "prefab_editor.hpp"

#include <fstream>

#include <imessentials/text_buffer.hpp>
#include <imgui.h>
#include <nodec_scene_serialization/entity_serializer.hpp>
#include <nodec_scene_serialization/systems/prefab_load_system.hpp>

namespace component_editors {

void PrefabEditor::on_inspector_gui(nodec_scene_serialization::components::Prefab &prefab, const nodec_scene_editor::InspectorGuiContext &context) {
    using namespace nodec;
    using namespace nodec_scene;
    using namespace nodec_scene_serialization;
    using namespace nodec_scene_serialization::components;
    using namespace nodec_scene_serialization::systems;

    auto entity = context.entity;
    auto &registry = context.registry;

    if (ImGui::Button("Save")) {
        [&]() {
            // TODO: add filter.
            auto serializable = EntitySerializer(serialization_).serialize(entity, scene_, [&](const SceneEntity &entt) {
                if (entt == entity) return true;
                auto *prefab = registry.try_get_component<Prefab>(entt);
                if (prefab != nullptr) return false;
                return true;
            });

            if (!serializable) {
                logger_->warn(__FILE__, __LINE__)
                    << "Failed to save.\n"
                       "details:\n"
                       "Target entity is invalid.";
                return;
            }
            std::ofstream out(Formatter() << resources_.resource_path() << "/" << prefab.source, std::ios::binary);
            if (!out) {
                logger_->warn(__FILE__, __LINE__)
                    << "Failed to save.\n"
                       "details:\n"
                       "Cannot open the file path. Make sure the directory exists.";
                return;
            }

            ArchiveContext context(resources_.registry());
            using Options = cereal::JSONOutputArchive::Options;
            cereal::UserDataAdapter<ArchiveContext, cereal::JSONOutputArchive> archive(context, out, Options(324, Options::IndentChar::space, 2u));

            try {
                archive(cereal::make_nvp("entity", *serializable));
            } catch (std::exception &e) {
                logger_->warn(__FILE__, __LINE__)
                    << "Failed to save.\n"
                       "details:/n"
                    << e.what();
                return;
            }

            logger_->info(__FILE__, __LINE__) << "Saved!";
        }();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        // Be careful. The following codes will change component structure while iterating.
        scene_.hierarchy_system().remove_all_children(entity);
        registry.remove_component<EntityBuilt>(entity);
        registry.emplace_component<PrefabLoadSystem::PrefabLoadActivity>(entity);
        // logging::InfoStream(__FILE__, __LINE__) << "Loaded!";
    }

    auto &buffer = imessentials::get_text_buffer(1024, prefab.source);
    if (ImGui::InputText("Source", buffer.data(), buffer.size())) {}
    prefab.source = buffer.data();
    ImGui::Checkbox("Active", &prefab.active);
}
} // namespace component_editors