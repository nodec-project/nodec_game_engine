#ifndef NODEC_GAME_ENGINE__SCENE_AUDIO__SCENE_AUDIO_SYSTEM_HPP_
#define NODEC_GAME_ENGINE__SCENE_AUDIO__SCENE_AUDIO_SYSTEM_HPP_

#include <nodec/logging/logging.hpp>
#include <nodec_scene/components/local_transform.hpp>
#include <nodec_scene/scene_registry.hpp>
#include <nodec_scene_audio/components/audio_listener.hpp>
#include <nodec_scene_audio/components/audio_source.hpp>

#include "../audio/audio_platform.hpp"

class SceneAudioSystem {
public:
    SceneAudioSystem(AudioPlatform &audio_platform,
                     nodec_scene::SceneRegistry &scene_registry);

public:
    void update(nodec_scene::SceneRegistry &registry);

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    AudioPlatform &audio_platform_;
};

#endif