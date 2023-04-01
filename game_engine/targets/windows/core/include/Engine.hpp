#pragma once

#include "Audio/AudioPlatform.hpp"
#include "Font/FontLibrary.hpp"
#include "ImguiManager.hpp"
#include "Input/KeyboardDeviceSystem.hpp"
#include "Input/MouseDeviceSystem.hpp"
#include "Physics/PhysicsSystemBackend.hpp"
#include "Rendering/SceneRenderer.hpp"
#include "Resources/ResourceLoader.hpp"
#include "Resources/ResourcesModuleBackend.hpp"
#include "SceneAudio/SceneAudioSystem.hpp"
#include "SceneSerialization/SceneSerializationBackend.hpp"
#include "ScreenHandler.hpp"
#include "Window.hpp"

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
#include <nodec_screen/impl/screen_module.hpp>
#include <nodec_world/impl/world_module.hpp>

#include <nodec/logging.hpp>

class Engine final {
public:
    Engine(nodec_application::impl::ApplicationImpl &app);

    ~Engine() {
        nodec::logging::InfoStream(__FILE__, __LINE__) << "[Engine] >>> Destructed.";

        // TODO: Consider to unload all modules before backends.

        // unload all scene entities.
        world_module_->scene().registry().clear();
    }

    void setup();

    void frame_begin();

    void frame_end();

    nodec_screen::impl::ScreenModule &screen_module() {
        return *screen_module_;
    }

    nodec_world::impl::WorldModule &world_module() {
        return *world_module_;
    }

    nodec_resources::impl::ResourcesImpl &resources_module() {
        return *resources_module_;
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

private:
    // imgui must be destroyed after window.
    std::unique_ptr<ImguiManager> imgui_manager_;
    std::unique_ptr<Window> window_;

    std::unique_ptr<FontLibrary> font_library_;

    std::unique_ptr<AudioPlatform> audio_platform_;

    std::shared_ptr<nodec_screen::impl::ScreenModule> screen_module_;
    std::unique_ptr<ScreenHandler> screen_handler_;

    std::shared_ptr<nodec_input::InputDevices> input_devices_;
    KeyboardDeviceSystem *keyboard_device_system_;
    MouseDeviceSystem *mouse_device_system_;

    std::shared_ptr<nodec_scene_serialization::impl::EntityLoaderImpl> entity_loader_;

    std::shared_ptr<ResourcesModuleBackend> resources_module_;

    std::shared_ptr<nodec_scene_serialization::SceneSerialization> scene_serialization_;
    std::unique_ptr<SceneSerializationBackend> scene_serialization_backend_;

    std::shared_ptr<nodec_world::impl::WorldModule> world_module_;

    std::unique_ptr<SceneRenderer> scene_renderer_;

    std::unique_ptr<nodec_rendering::systems::VisibilitySystem> visibility_system_;

    std::unique_ptr<SceneAudioSystem> scene_audio_system_;

    std::shared_ptr<PhysicsSystemBackend> physics_system_;

    std::unique_ptr<SceneRenderingContext> scene_rendering_context_;
};

#if CEREAL_THREAD_SAFE != 1
#    error The macro 'CEREAL_THREAD_SAFE' must be set to '1' for the whole project.
#endif