#include "editor.hpp"

#include <ImGuizmo.h>
#include <nodec_scene_serialization/components/non_serialized.hpp>
#include <nodec_physics/components/static_rigid_body.hpp>
#include <nodec_physics/components/trigger_body.hpp>

#include "EditorConfig.hpp"

#include "component_editors/audio_source_editor.hpp"
#include "component_editors/camera_editor.hpp"
#include "component_editors/directional_light_editor.hpp"
#include "component_editors/image_renderer_editor.hpp"
#include "component_editors/local_transform_editor.hpp"
#include "component_editors/mesh_renderer_editor.hpp"
#include "component_editors/name_editor.hpp"
#include "component_editors/non_visible_editor.hpp"
#include "component_editors/physics_shape_editor.hpp"
#include "component_editors/point_light_editor.hpp"
#include "component_editors/post_processing_editor.hpp"
#include "component_editors/prefab_editor.hpp"
#include "component_editors/rigid_body_editor.hpp"
#include "component_editors/scene_lighting_editor.hpp"
#include "component_editors/text_renderer_editor.hpp"
#include "editor_windows/asset_import_window.hpp"
#include "editor_windows/control_window.hpp"
#include "editor_windows/entity_inspector_window.hpp"
#include "editor_windows/log_window.hpp"
#include "editor_windows/material_editor_window.hpp"
#include "editor_windows/scene_hierarchy_window.hpp"
#include "editor_windows/scene_view_window.hpp"

Editor::Editor(Engine *engine)
    : logger_(nodec::logging::get_logger("editor")), engine_{engine} {
    using namespace nodec;
    using namespace imessentials;
    using namespace nodec_scene_editor;

    editor_gui_.reset(new EditorGui(engine->resources_module()));

    window_manager().register_window<ControlWindow>([=]() {
        return std::make_unique<ControlWindow>(this);
    });

    window_manager().register_window<SceneViewWindow>([=]() {
        return std::make_unique<SceneViewWindow>(engine->window().graphics(),
                                                 engine->world_module().scene(), engine->scene_renderer(),
                                                 engine->resources_module(),
                                                 *scene_gizmo_, component_registry_impl());
    });

    window_manager().register_window<SceneHierarchyWindow>([=]() {
        return std::make_unique<SceneHierarchyWindow>(engine->world_module().scene(), engine->scene_serialization());
    });

    window_manager().register_window<EntityInspectorWindow>([=]() {
        return std::make_unique<EntityInspectorWindow>(engine->world_module().scene().registry(),
                                                       component_registry_impl());
    });

    window_manager().register_window<LogWindow>([=]() {
        return std::make_unique<LogWindow>();
    });

    window_manager().register_window<MaterialEditorWindow>([=]() {
        return std::make_unique<MaterialEditorWindow>(engine->resources_module());
    });

    window_manager().register_window<AssetImportWindow>([=]() {
        return std::make_unique<AssetImportWindow>(engine->resources_module().resource_path(),
                                                   &engine->world_module().scene(),
                                                   &engine->resources_module().registry());
    });

    register_menu_item("Window/Control", [=]() {
        auto &window = window_manager().get_window<ControlWindow>();
        window.focus();
    });

    register_menu_item("Window/Scene View", [=]() {
        auto &window = window_manager().get_window<SceneViewWindow>();
        window.focus();
    });

    register_menu_item("Window/Scene Hierarchy", [=]() {
        auto &window = window_manager().get_window<SceneHierarchyWindow>();
        window.focus();
    });

    register_menu_item("Window/Entity Inspector", [=]() {
        auto &window = window_manager().get_window<EntityInspectorWindow>();
        window.focus();
    });

    register_menu_item("Window/Log", [&]() {
        auto &window = window_manager().get_window<LogWindow>();
        window.focus();
    });

    register_menu_item("Window/Material Editor", [&]() {
        auto &window = window_manager().get_window<MaterialEditorWindow>();
        window.focus();
    });

    register_menu_item("Window/Asset Importer", [&]() {
        auto &window = window_manager().get_window<AssetImportWindow>();
        window.focus();
    });

    {
        using namespace component_editors;

        using namespace nodec_scene::components;
        using namespace nodec_rendering::components;
        using namespace nodec_scene_audio::components;
        using namespace nodec_scene_serialization::components;
        using namespace nodec_physics::components;

        component_registry().register_component<AudioSource, AudioSourceEditor>("Audio Source", *editor_gui_);
        component_registry().register_component<Camera, CameraEditor>("Camera");
        component_registry().register_component<DirectionalLight, DirectionalLightEditor>("Directional Light");
        component_registry().register_component<ImageRenderer, ImageRendererEditor>("Image Renderer", *editor_gui_);
        component_registry().register_component<LocalTransform, LocalTransformEditor>("Local Transform");
        component_registry().register_component<MeshRenderer, MeshRendererEditor>("Mesh Renderer", *editor_gui_, engine->resources_module());
        component_registry().register_component<Name, NameEditor>("Name");
        component_registry().register_component<NonSerialized>("Non Serialized");
        component_registry().register_component<NonVisible, NonVisibleEditor>("Non Visible");
        component_registry().register_component<PhysicsShape, PhysicsShapeEditor>("Physics Shape");
        component_registry().register_component<PointLight, PointLightEditor>("Point Light");
        component_registry().register_component<PostProcessing, PostProcessingEditor>("Post Processing", *editor_gui_, engine->resources_module());
        component_registry().register_component<Prefab, PrefabEditor>("Prefab", engine->resources_module(), engine->world_module().scene(), engine->scene_serialization());
        component_registry().register_component<RigidBody, RigidBodyEditor>("Rigid Body");
        component_registry().register_component<SceneLighting, SceneLightingEditor>("Scene Lighting", *editor_gui_);
        component_registry().register_component<StaticRigidBody>("Static Rigid Body");
        component_registry().register_component<TextRenderer, TextRendererEditor>("Text Renderer", *editor_gui_);
        component_registry().register_component<TriggerBody>("Trigger Body");
    }

    [=]() {
        std::ifstream file("editor-config.json", std::ios::binary);
        if (!file) return;

        EditorConfig config;

        try {
            cereal::JSONInputArchive archive(file);
            archive(config);
        } catch (std::exception &e) {
            logger_->warn(__FILE__, __LINE__)
                << "Failed to load editor configuration.\n"
                << "details: \n"
                << e.what();
            return;
        }

        if (!config.resource_path.empty()) {
            engine->resources_module().set_resource_path(config.resource_path);
        }

        if (!config.font.path.empty()) {
            auto &io = ImGui::GetIO();
            io.Fonts->AddFontFromFileTTF(config.font.path.c_str(), config.font.pixel_size, NULL, io.Fonts->GetGlyphRangesJapanese());
        }
    }();
}

void Editor::setup() {
    scene_gizmo_.reset(new SceneGizmoImpl(engine_->world_module().scene(), engine_->resources_module()));

    // TODO: Restore the previous workspace.
    //  * Last opened windows.
    using namespace nodec_scene_editor;

    window_manager().get_window<ControlWindow>();
    window_manager().get_window<SceneViewWindow>();
    window_manager().get_window<SceneHierarchyWindow>();
    window_manager().get_window<LogWindow>();
    window_manager().get_window<MaterialEditorWindow>();
    window_manager().get_window<AssetImportWindow>();
    window_manager().get_window<EntityInspectorWindow>();
}

void Editor::update() {
    switch (state_) {
    case State::Playing:
        engine_->world_module().step();
        break;

    case State::Paused:

        if (do_one_step_) {
            engine_->world_module().step(1 / 60.0f);
            do_one_step_ = false;
        }

        break;
    }

    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        bool open = true;
        ImGui::Begin("Editor DockSpace", &open, window_flags);

        ImGuiDockNodeFlags dock_space_flags = ImGuiDockNodeFlags_None;
        ImGui::DockSpace(ImGui::GetID("editor-dock-space"), ImVec2(0.0f, 0.0f), dock_space_flags);

        imessentials::impl::show_menu_bar();

        ImGui::End();
    }

    ImGuizmo::BeginFrame();

    window_manager_impl().update_windows();

    {
        bool open = true;
        ImGui::ShowDemoWindow(&open);
    }
}