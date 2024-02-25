#pragma once

#include <chrono>
#include <memory>

#include "../audio/source_voice.hpp"
#include "audio_clip_backend.hpp"

/**
 * ECS Component
 */
struct AudioSourceActivity {
    enum class State {
        Stopped,
        Playing
    };

    State state{State::Stopped};
    std::shared_ptr<AudioClipBackend> pClip;
    std::unique_ptr<SourceVoice> pVoice;
    std::chrono::milliseconds playBeginTime;
};