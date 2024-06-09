#ifndef NODEC_GAME_ENGINE__ENGINE_HPP_
#define NODEC_GAME_ENGINE__ENGINE_HPP_

#include "Font/FontLibrary.hpp"
#include "ImguiManager.hpp"
#include "audio/audio_platform.hpp"
#include "input/keyboard_device_system.hpp"
#include "input/mouse_device_system.hpp"
#include "physics/physics_system_backend.hpp"
#include "rendering/scene_renderer.hpp"
#include "resources/resources_backend.hpp"
#include "scene_audio/scene_audio_system.hpp"
#include "scene_serialization/scene_serialization_backend.hpp"
#include "screen/screen_backend.hpp"
#include "window.hpp"

#include <nodec/logging/logging.hpp>
#include <nodec_animation/component_registry.hpp>
#include <nodec_animation/systems/animator_system.hpp>
#include <nodec_application/impl/application_impl.hpp>
#include <nodec_input/input_devices.hpp>
#include <nodec_input/keyboard/impl/keyboard_device.hpp>
#include <nodec_input/mouse/impl/mouse_device.hpp>
#include <nodec_physics/systems/physics_system.hpp>
#include <nodec_rendering/systems/visibility_system.hpp>
#include <nodec_resources/impl/resources_impl.hpp>
#include <nodec_scene/scene.hpp>
#include <nodec_scene/systems/transform_system.hpp>
#include <nodec_scene_serialization/impl/entity_loader_impl.hpp>
#include <nodec_scene_serialization/scene_serialization.hpp>
#include <nodec_scene_serialization/systems/prefab_load_system.hpp>
#include <nodec_world/impl/world_impl.hpp>

class Engine final {
public:
    Engine(nodec_application::impl::ApplicationImpl &app);

    ~Engine();

    void setup();

    void frame_begin();

    void frame_end();

    nodec_screen::Screen &screen() {
        return *screen_;
    }

    nodec_world::impl::WorldImpl &world_module() {
        return *world_;
    }

    nodec_resources::impl::ResourcesImpl &resources() {
        return *resources_;
    }

    nodec_scene_serialization::SceneSerialization &scene_serialization() {
        return *scene_serialization_;
    }
    AudioPlatform &audio_platform() {
        return *audio_platform_;
    }

    Window &window() {
        return *window_;
    }

    SceneRenderer &scene_renderer() {
        return *scene_renderer_;
    }

    SceneRenderingContext &scene_rendering_context() {
        return *scene_rendering_context_;
    }

private:
    void on_stepped(nodec_world::World &);

private:
    std::shared_ptr<nodec::logging::Logger> logger_;

    // imgui must be destroyed after window.
    std::unique_ptr<ImguiManager> imgui_manager_;
    std::unique_ptr<Window> window_;

    std::unique_ptr<FontLibrary> font_library_;

    std::unique_ptr<AudioPlatform> audio_platform_;

    std::shared_ptr<ScreenBackend> screen_;

    std::shared_ptr<nodec_input::InputDevices> input_devices_;
    KeyboardDeviceSystem *keyboard_device_system_;
    MouseDeviceSystem *mouse_device_system_;

    std::shared_ptr<nodec_scene_serialization::impl::EntityLoaderImpl> entity_loader_;

    std::shared_ptr<ResourcesBackend> resources_;

    std::shared_ptr<nodec_scene_serialization::SceneSerialization> scene_serialization_;
    std::unique_ptr<SceneSerializationBackend> scene_serialization_backend_;

    std::shared_ptr<nodec_world::impl::WorldImpl> world_;

    std::unique_ptr<SceneRenderer> scene_renderer_;

    std::unique_ptr<nodec_rendering::systems::VisibilitySystem> visibility_system_;

    std::unique_ptr<SceneAudioSystem> scene_audio_system_;

    std::shared_ptr<PhysicsSystemBackend> physics_system_;

    std::unique_ptr<SceneRenderingContext> scene_rendering_context_;
    std::unique_ptr<nodec_scene_serialization::systems::PrefabLoadSystem> prefab_load_system_;

    std::shared_ptr<nodec_animation::ComponentRegistry> animation_component_registry_;
    std::unique_ptr<nodec_animation::systems::AnimatorSystem> animator_system_;
};

#if CEREAL_THREAD_SAFE != 1
#    error The macro 'CEREAL_THREAD_SAFE' must be set to '1' for the whole project.
#endif

#endif