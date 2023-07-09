#pragma once

#include "../Audio/AudioPlatform.hpp"
#include "AudioSourceActivity.hpp"

#include <nodec_scene/components/local_transform.hpp>
#include <nodec_scene/scene_registry.hpp>
#include <nodec_scene_audio/components/audio_source.hpp>
#include <nodec_scene_audio/components/audio_listener.hpp>

class SceneAudioSystem {
public:
    SceneAudioSystem(AudioPlatform *audioPlatform, nodec_scene::SceneRegistry *pSceneRegistry) // scene_registry
        : audio_platform_(audioPlatform) {
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
    AudioPlatform *audio_platform_; // audio_platform_;
};