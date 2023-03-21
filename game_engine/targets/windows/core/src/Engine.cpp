#include <Engine.hpp>

Engine::Engine(nodec_application::impl::ApplicationImpl &app) {
    using namespace nodec_world;
    using namespace nodec_world::impl;
    using namespace nodec_screen::impl;
    using namespace nodec_screen;
    using namespace nodec_input;
    using namespace nodec_resources;
    using namespace nodec_scene_serialization;
    using namespace nodec_physics::systems;

    nodec::logging::InfoStream(__FILE__, __LINE__) << "[Engine] >>> Created!";

    imgui_manager_.reset(new ImguiManager);

    font_library_.reset(new FontLibrary);

    // --- screen ---
    screen_module_.reset(new ScreenModule());

    screen_handler_.reset(new ScreenHandler(screen_module_.get()));
    screen_handler_->BindHandlersOnBoot();

    // --- world ---
    world_module_.reset(new WorldModule());

    // --- input ---
    input_devices_.reset(new nodec_input::InputDevices());

    keyboard_device_system_ = &input_devices_->emplace_device_system<KeyboardDeviceSystem>();
    mouse_device_system_ = &input_devices_->emplace_device_system<MouseDeviceSystem>();

    // --- resources ---
    resources_module_.reset(new ResourcesModuleBackend());

    resources_module_->setup_on_boot();

    // --- scene serialization ---
    scene_serialization_module_.reset(new SceneSerializationModuleBackend(&resources_module_->registry()));
    entity_loader_.reset(new nodec_scene_serialization::impl::EntityLoaderImpl(*scene_serialization_module_, world_module_->scene(), resources_module_->registry()));

    // --- others ---
    physics_system_.reset(new PhysicsSystemBackend(*world_module_));

    visibility_system_.reset(new nodec_rendering::systems::VisibilitySystem(world_module_->scene()));

    // --- Export the services to application.
    app.add_service<Screen>(screen_module_);
    app.add_service<World>(world_module_);
    app.add_service<InputDevices>(input_devices_);
    app.add_service<Resources>(resources_module_);
    app.add_service<SceneSerialization>(scene_serialization_module_);
    app.add_service<EntityLoader>(entity_loader_);
    app.add_service<PhysicsSystem>(physics_system_);
}

void Engine::setup() {
    using namespace nodec;

    window_.reset(new Window(
        screen_module_->internal_size.x, screen_module_->internal_size.y,
        screen_module_->internal_resolution.x, screen_module_->internal_resolution.y,
        unicode::utf8to16<std::wstring>(screen_module_->internal_title).c_str(),
        &keyboard_device_system_->device(), &mouse_device_system_->device()));

    screen_handler_->Setup(window_.get());

    resources_module_->setup_on_runtime(&window_->graphics(), font_library_.get());

    scene_renderer_.reset(new SceneRenderer(&window_->graphics(), resources_module_->registry()));

    audio_platform_.reset(new AudioPlatform());

    scene_audio_system_.reset(new SceneAudioSystem(audio_platform_.get(), &world_module_->scene().registry()));

    scene_rendering_context_.reset(new SceneRenderingContext(window_->graphics().width(), window_->graphics().height(), &window_->graphics()));

    world_module_->stepped().connect([&](auto &world) {
        scene_audio_system_->UpdateAudio(world_module_->scene().registry());
    });
}

void Engine::frame_begin() {
    window_->graphics().BeginFrame();
}

void Engine::frame_end() {
    using namespace nodec::entities;
    using namespace nodec_scene::components;

    {
        auto root = world_module_->scene().hierarchy_system().root_hierarchy().first;
        while (root != null_entity) {
            nodec_scene::systems::update_transform(world_module_->scene().registry(), root);
            root = world_module_->scene().registry().get_component<Hierarchy>(root).next;
        }
    }

    scene_renderer_->Render(world_module_->scene(),
                            window_->graphics().render_target_view(),
                            *scene_rendering_context_);

    window_->graphics().EndFrame();

    entity_loader_->update();
}
