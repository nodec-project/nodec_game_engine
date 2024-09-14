#include "editor.hpp"

#include <ImGuizmo.h>
#include <nodec_physics/components/static_rigid_body.hpp>
#include <nodec_physics/components/trigger_body.hpp>
#include <nodec_scene_serialization/components/non_serialized.hpp>

#include "EditorConfig.hpp"

#include "component_editors/animator_editor.hpp"
#include "component_editors/audio_listener_editor.hpp"
#include "component_editors/audio_source_editor.hpp"
#include "component_editors/camera_editor.hpp"
#include "component_editors/collision_filter_editor.hpp"
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
#include "editor_windows/geometry_buffer_inspector_window.hpp"
#include "editor_windows/log_window.hpp"
#include "editor_windows/material_editor_window.hpp"
#include "editor_windows/scene_hierarchy_window.hpp"
#include "editor_windows/scene_view_window.hpp"

Editor::Editor(Engine *engine)
    : logger_(nodec::logging::get_logger("editor")), engine_{engine} {
    using namespace nodec;
    using namespace imessentials;
    using namespace nodec_scene_editor;

    editor_gui_.reset(new EditorGui(engine->resources()));

    window_manager().register_window<ControlWindow>([=]() {
        return std::make_unique<ControlWindow>(this);
    });

    window_manager().register_window<SceneViewWindow>([=]() {
        return std::make_unique<SceneViewWindow>(engine->window().graphics(),
                                                 engine->world_module().scene(), engine->scene_renderer(),
                                                 engine->resources(),
                                                 *scene_gizmo_, component_registry_impl());
    });

    window_manager().register_window<SceneHierarchyWindow>([=]() {
        return std::make_unique<SceneHierarchyWindow>(engine->world_module().scene(), engine->scene_serialization());
    });

    window_manager().register_window<EntityInspectorWindow>([=]() {
        return std::make_unique<EntityInspectorWindow>(engine->world_module().scene().registry(),
                                                       component_registry_impl(),
                                                       engine->scene_serialization());
    });

    window_manager().register_window<LogWindow>([=]() {
        return std::make_unique<LogWindow>();
    });

    window_manager().register_window<MaterialEditorWindow>([=]() {
        return std::make_unique<MaterialEditorWindow>(engine->resources());
    });

    window_manager().register_window<AssetImportWindow>([=]() {
        return std::make_unique<AssetImportWindow>(engine->resources().resource_path(),
                                                   &engine->world_module().scene(),
                                                   &engine->resources().registry());
    });

    window_manager().register_window<GeometryBufferInspectorWindow>([=]() {
        return std::make_unique<GeometryBufferInspectorWindow>(engine->scene_rendering_context());
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

    register_menu_item("Window/Geometry Buffer Inspector", [&]() {
        auto &window = window_manager().get_window<GeometryBufferInspectorWindow>();
        window.focus();
    });

    {
        using namespace component_editors;

        {
            using namespace nodec_rendering::components;

            component_registry().register_component<Camera, CameraEditor>("Camera");
            component_registry().register_component<DirectionalLight, DirectionalLightEditor>("Directional Light");
            component_registry().register_component<ImageRenderer, ImageRendererEditor>("Image Renderer", *editor_gui_);
            component_registry().register_component<MeshRenderer, MeshRendererEditor>("Mesh Renderer", *editor_gui_, engine->resources());
            component_registry().register_component<NonVisible, NonVisibleEditor>("Non Visible");
            component_registry().register_component<PointLight, PointLightEditor>("Point Light");
            component_registry().register_component<PostProcessing, PostProcessingEditor>("Post Processing", *editor_gui_, engine->resources());
            component_registry().register_component<SceneLighting, SceneLightingEditor>("Scene Lighting", *editor_gui_);
            component_registry().register_component<TextRenderer, TextRendererEditor>("Text Renderer", *editor_gui_);
        }

        {
            using namespace nodec_scene::components;
            component_registry().register_component<LocalTransform, LocalTransformEditor>("Local Transform");
            component_registry().register_component<Name, NameEditor>("Name");
        }

        {
            using namespace nodec_animation::components;
            component_registry().register_component<Animator, AnimatorEditor>("Animator", *editor_gui_);
            component_registry().register_component<AnimatorStart>("Animator Start");
            component_registry().register_component<AnimatorStop>("Animator Stop");
        }

        {
            using namespace nodec_scene_audio::components;
            component_registry().register_component<AudioSource, AudioSourceEditor>("Audio Source", *editor_gui_);
            component_registry().register_component<AudioListener>("Audio Listener");
            component_registry().register_component<AudioPlay>("Audio Play");
            component_registry().register_component<AudioStop>("Audio Stop");
        }

        {
            using namespace nodec_physics::components;
            component_registry().register_component<PhysicsShape, PhysicsShapeEditor>("Physics Shape");
            component_registry().register_component<RigidBody, RigidBodyEditor>("Rigid Body");
            component_registry().register_component<StaticRigidBody>("Static Rigid Body");
            component_registry().register_component<TriggerBody>("Trigger Body");
            component_registry().register_component<CollisionFilter, CollisionFilterEditor>("Collision Filter");
        }

        {
            using namespace nodec_scene_serialization::components;
            component_registry().register_component<NonSerialized>("Non Serialized");
            component_registry().register_component<Prefab, PrefabEditor>("Prefab", engine->resources(), engine->world_module().scene(), engine->scene_serialization());
        }
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
            engine->resources().set_resource_path(config.resource_path);
        }

        if (!config.font.path.empty()) {
            auto &io = ImGui::GetIO();
            io.Fonts->AddFontFromFileTTF(config.font.path.c_str(), config.font.pixel_size, NULL, io.Fonts->GetGlyphRangesJapanese());
        }
    }();
}

void Editor::setup() {
    scene_gizmo_.reset(new SceneGizmoImpl(engine_->world_module().scene(), engine_->resources()));

    // TODO: Restore the previous workspace.
    //  * Last opened windows.
    using namespace nodec_scene_editor;

    window_manager().get_window<ControlWindow>();
    window_manager().get_window<SceneViewWindow>();
    window_manager().get_window<LogWindow>();
    window_manager().get_window<MaterialEditorWindow>();
    window_manager().get_window<AssetImportWindow>();
    window_manager().get_window<GeometryBufferInspectorWindow>();
    window_manager().get_window<EntityInspectorWindow>();
    window_manager().get_window<SceneHierarchyWindow>();
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
