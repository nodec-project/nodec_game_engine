#pragma once

#include "../audio/audio_platform.hpp"
#include "audio_source_activity.hpp"

#include <nodec/logging/logging.hpp>
#include <nodec_scene/components/local_transform.hpp>
#include <nodec_scene/scene_registry.hpp>
#include <nodec_scene_audio/components/audio_listener.hpp>
#include <nodec_scene_audio/components/audio_source.hpp>

class SceneAudioSystem {
public:
    SceneAudioSystem(AudioPlatform *audio_platform, nodec_scene::SceneRegistry *pSceneRegistry) // scene_registry
        : logger_(nodec::logging::get_logger("engine.scene-audio.scene-audio-system")), audio_platform_(audio_platform) {
        using namespace nodec_scene_audio::components;
        pSceneRegistry->component_constructed<AudioSource>().connect(
            [&](auto &registry, auto entity) {
                registry.emplace_component<AudioSourceActivity>(entity);
            });
    }

public:
    // update_audio();
    void UpdateAudio(nodec_scene::SceneRegistry &registry);

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    AudioPlatform *audio_platform_; // audio_platform_;
};