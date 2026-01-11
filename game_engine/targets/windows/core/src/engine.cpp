#include <engine.hpp>

// // Windows.hをuWebSocketsより前にインクルードして競合を防ぐ
// #define NOMINMAX
// #define WIN32_LEAN_AND_MEAN
// #include <Windows.h>

#include <nodec/stopwatch.hpp>

#include "animation/animation.hpp"

Engine::Engine(nodec_application::impl::ApplicationImpl &app)
    : logger_(nodec::logging::get_logger("engine")) {
    using namespace nodec_world;
    using namespace nodec_world::impl;
    using namespace nodec_screen;
    using namespace nodec_input;
    using namespace nodec_resources;
    using namespace nodec_scene_serialization;
    using namespace nodec_physics::systems;

    logger_->info(__FILE__, __LINE__) << "Created!";

    // --- 通常のエンジン初期化処理 ---
    imgui_manager_.reset(new ImguiManager);

    font_library_.reset(new FontLibrary);

    // --- screen ---
    screen_.reset(new ScreenBackend());

    // --- world ---
    world_.reset(new WorldImpl());

    // --- input ---
    input_devices_.reset(new nodec_input::InputDevices());

    keyboard_device_system_ = &input_devices_->emplace_device_system<KeyboardDeviceSystem>();
    mouse_device_system_ = &input_devices_->emplace_device_system<MouseDeviceSystem>();

    // --- resources ---
    resources_.reset(new ResourcesBackend());

    resources_->setup_on_boot();

    // --- scene serialization ---
    scene_serialization_.reset(new SceneSerialization());
    scene_serialization_backend_.reset(new SceneSerializationBackend(&resources_->registry(), *scene_serialization_));
    entity_loader_.reset(new nodec_scene_serialization::impl::EntityLoaderImpl(*scene_serialization_, world_->scene(), resources_->registry()));

    // --- others ---
    physics_system_.reset(new PhysicsSystemBackend(*world_));

    visibility_system_.reset(new nodec_rendering::systems::VisibilitySystem(world_->scene()));
    prefab_load_system_.reset(new nodec_scene_serialization::systems::PrefabLoadSystem(world_->scene(), *entity_loader_));

    animation_component_registry_.reset(new nodec_animation::ComponentRegistry());
    setup_animation_component_registry(*animation_component_registry_);
    animator_system_.reset(new nodec_animation::systems::AnimatorSystem(*animation_component_registry_));

    world_->stepped().connect([=](nodec_world::World &world) {
        on_stepped(world);
    });

    // --- Mouse event tracking for UI
    mouse_event_connection_ = mouse_device_system_->device().mouse_event().connect(
        [this](const nodec_input::mouse::MouseEvent &event) {
            using namespace nodec_input::mouse;
            mouse_state_.position = {static_cast<float>(event.position.x), static_cast<float>(event.position.y)};

            if (event.type == MouseEvent::Type::Press && event.button == MouseButton::Left) {
                mouse_state_.button_down = true;
                mouse_state_.button_pressed = true;
            } else if (event.type == MouseEvent::Type::Release && event.button == MouseButton::Left) {
                mouse_state_.button_down = false;
                mouse_state_.button_released = true;
            }
        });

    // --- Export the services to application.
    app.add_service<Screen>(screen_);
    app.add_service<World>(world_);
    app.add_service<InputDevices>(input_devices_);
    app.add_service<Resources>(resources_);
    app.add_service<SceneSerialization>(scene_serialization_);
    app.add_service<EntityLoader>(entity_loader_);
    app.add_service<PhysicsSystem>(physics_system_);
    app.add_service<nodec_animation::ComponentRegistry>(animation_component_registry_);
}

Engine::~Engine() {
    logger_->info(__FILE__, __LINE__) << "Destroyed!";

    // TODO: Consider to unload all modules before backends.

    // unload all scene entities.
    world_->scene().registry().clear();
}

void Engine::setup(GraphicsDevice &shared_device) {
    using namespace nodec;

    shared_graphics_device_ = &shared_device;

    window_.reset(new Window(
        screen_->size().x, screen_->size().y,
        screen_->resolution().x, screen_->resolution().y,
        unicode::utf8to16<std::wstring>(screen_->title()).c_str(),
        &keyboard_device_system_->device(), &mouse_device_system_->device(),
        shared_device));

    screen_->setup(window_.get());

    resources_->setup_on_runtime(window_->graphics(), *font_library_, *scene_serialization_);

    scene_renderer_.reset(new SceneRenderer(world_->scene(), window_->graphics(), resources_->registry()));

    audio_platform_.reset(new AudioPlatform());

    scene_audio_system_.reset(new SceneAudioSystem(*audio_platform_, world_->scene().registry()));

    scene_rendering_context_.reset(new SceneRenderingContext(window_->graphics().width(), window_->graphics().height(), window_->graphics()));

    logger_->info(__FILE__, __LINE__) << "Engine setup with shared GraphicsDevice.";
}

void Engine::on_stepped(nodec_world::World &world) {
    scene_audio_system_->update(world_->scene().registry());
    animator_system_->update(world_->scene().registry(), world.clock().delta_time());

    // UI Raycaster System
    {
        nodec_ui::systems::UiRaycasterSystem::InputState input_state;
        input_state.mouse_position = mouse_state_.position;
        input_state.screen_size = {
            static_cast<float>(screen_->resolution().x),
            static_cast<float>(screen_->resolution().y)};
        input_state.mouse_button_down = mouse_state_.button_down;
        input_state.mouse_button_pressed = mouse_state_.button_pressed;
        input_state.mouse_button_released = mouse_state_.button_released;

        ui_raycaster_system_.update(world_->scene().registry(), *physics_system_, input_state);

        // Reset edge-triggered flags
        mouse_state_.button_pressed = false;
        mouse_state_.button_released = false;
    }
}

void Engine::frame_begin() {
    window_->graphics().begin_frame();
}

void Engine::frame_end() {
    using namespace nodec::entities;
    using namespace nodec_scene::components;

    // Emplacing the entities then update these transforms.
    prefab_load_system_->update();
    entity_loader_->update();

    // Update transform
    {
        auto root = world_->scene().hierarchy_system().root_hierarchy().first;
        while (root != null_entity) {
            nodec_scene::systems::update_transform(world_->scene().registry(), root);
            root = world_->scene().registry().get_component<Hierarchy>(root).next;
        }
    }

    scene_renderer_->render(world_->scene(),
                            window_->graphics().render_target_view(),
                            *scene_rendering_context_);

    window_->graphics().end_frame();
}
