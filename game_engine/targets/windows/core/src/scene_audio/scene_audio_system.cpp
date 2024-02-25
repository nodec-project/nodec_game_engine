#include <scene_audio/scene_audio_system.hpp>

#include <chrono>
#include <memory>

#include <nodec_scene/components/local_to_world.hpp>

#include <audio/source_voice.hpp>
#include <scene_audio/audio_clip_backend.hpp>

struct AudioListenerActivity {
    X3DAUDIO_LISTENER listener_backend;
};

struct AudioSourceActivity {
    enum class State {
        Stopped,
        Playing
    };

    State state{State::Stopped};
    std::shared_ptr<AudioClipBackend> clip;
    std::unique_ptr<SourceVoice> voice;
    std::chrono::milliseconds play_begin_time;

    X3DAUDIO_EMITTER emitter_backend = {};
};

namespace {
void handle_exception(std::shared_ptr<nodec::logging::Logger> logger, nodec_scene::SceneEntity entity) {
    nodec::logging::LogStream error(logger, nodec::logging::Level::Error, __FILE__, __LINE__);

    error << "Audio Error. \n"
             "entity: "
          << std::hex << "0x" << entity << std::dec << "\n"
          << "details:\n";
    try {
        throw;
    } catch (std::exception &e) {
        error << e.what();
    } catch (...) {
        error << "Unknown";
    }
}

} // namespace

SceneAudioSystem::SceneAudioSystem(AudioPlatform &audio_platform,
                                   nodec_scene::SceneRegistry &scene_registry)
    : logger_(nodec::logging::get_logger("engine.scene-audio.scene-audio-system")),
      audio_platform_(audio_platform) {
}

void SceneAudioSystem::update(nodec_scene::SceneRegistry &scene_registry) {
    using namespace nodec;
    using namespace nodec_scene_audio::components;
    using namespace nodec_scene::components;
    using namespace nodec_scene;
    using namespace Exceptions;

    scene_registry.view<AudioListener, LocalToWorld>(nodec::type_list<AudioListenerActivity>{}).each([&](SceneEntity entity, AudioListener &, LocalToWorld &local_to_world) {
        auto &activity = scene_registry.emplace_component<AudioListenerActivity>(entity).first;
        activity.listener_backend = {};
        const auto &position = local_to_world.value.c[3];
        activity.listener_backend.Position = {position.x, position.y, position.z};
    });

    const X3DAUDIO_LISTENER *main_listener = nullptr;

    scene_registry.view<AudioListener, AudioListenerActivity, LocalToWorld>()
        .each([&](SceneEntity entity,
                  AudioListener &listener, AudioListenerActivity &listener_activity,
                  LocalToWorld &local_to_world) {
            auto &listener_backend = listener_activity.listener_backend;

            const auto position = local_to_world.value.c[3];
            const auto previous_position = nodec::Vector4f(
                listener_backend.Position.x, listener_backend.Position.y, listener_backend.Position.z, 1.0f);

            const auto &orient_front = local_to_world.value * Vector4f{0, 0, 1, 1};
            const auto &orient_top = local_to_world.value * Vector4f{0, 1, 0, 1};
            const auto &orient_right = local_to_world.value * Vector4f{1, 0, 0, 1};

            listener_backend.OrientFront = {orient_front.x, orient_front.y, orient_front.z};
            listener_backend.OrientTop = {orient_top.x, orient_top.y, orient_top.z};

            listener_backend.Position = {position.x, position.y, position.z};

            const auto velocity = position - previous_position;
            listener_backend.Velocity = {velocity.x, velocity.y, velocity.z};

            main_listener = &listener_backend;
        });

    {
        auto view = scene_registry.view<AudioListenerActivity>(nodec::type_list<AudioListener>{});
        scene_registry.remove_components<AudioListenerActivity>(view.begin(), view.end());
    }

    scene_registry.view<AudioSource, LocalToWorld>(nodec::type_list<AudioSourceActivity>{})
        .each([&](SceneEntity entity, AudioSource &source, LocalToWorld &local_to_world) {
            auto &activity = scene_registry.emplace_component<AudioSourceActivity>(entity).first;
            activity.emitter_backend = {};
            const auto position = local_to_world.value.c[3];
            activity.emitter_backend.Position = {position.x, position.y, position.z};
        });

    // Update emitter information.
    scene_registry.view<AudioSource, AudioSourceActivity, LocalToWorld>()
        .each([&](SceneEntity entity, AudioSource &source, AudioSourceActivity &activity, LocalToWorld &local_to_world) {
            auto &emitter_backend = activity.emitter_backend;

            const auto position = local_to_world.value.c[3];
            const auto previous_position = nodec::Vector4f(
                emitter_backend.Position.x, emitter_backend.Position.y, emitter_backend.Position.z, 1.0f);

            const auto &orient_front = local_to_world.value * Vector4f{0, 0, 1, 1};
            const auto &orient_top = local_to_world.value * Vector4f{0, 1, 0, 1};

            emitter_backend.OrientFront = {orient_front.x, orient_front.y, orient_front.z};
            emitter_backend.OrientTop = {orient_top.x, orient_top.y, orient_top.z};
            emitter_backend.Position = {position.x, position.y, position.z};

            const auto velocity = position - previous_position;
            emitter_backend.Velocity = {velocity.x, velocity.y, velocity.z};

            emitter_backend.CurveDistanceScaler = 1.0f;
            // emitter_backend.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
            emitter_backend.pVolumeCurve = nullptr;
        });

    // Play audio source.
    [&]() {
        auto view = scene_registry.view<AudioSource, AudioSourceActivity, AudioPlay>();
        view.each([&](SceneEntity entity, AudioSource &source, AudioSourceActivity &activity, AudioPlay &play) {
            try {
                if (activity.state == AudioSourceActivity::State::Playing) return;

                activity.clip = std::static_pointer_cast<AudioClipBackend>(source.clip);
                if (!activity.clip) return;

                if (!activity.voice || activity.voice->GetWfx() != activity.clip->wfx()) {
                    activity.voice.reset(new SourceVoice(&audio_platform_, activity.clip->wfx()));
                }

                const auto &wfx = activity.clip->wfx();

                int play_begin = source.position.count() * wfx.nSamplesPerSec / 1000;
                int total_sample_frames = activity.clip->samples().size() / wfx.nBlockAlign;

                if (play_begin < 0 || total_sample_frames <= play_begin) return;

                XAUDIO2_BUFFER buffer = {};
                buffer.pAudioData = activity.clip->samples().data();
                buffer.Flags = XAUDIO2_END_OF_STREAM;
                buffer.AudioBytes = activity.clip->samples().size();
                buffer.LoopCount = source.loop ? XAUDIO2_LOOP_INFINITE : 0;

                buffer.PlayBegin = play_begin;
                activity.play_begin_time = source.position;

                ThrowIfFailed(activity.voice->GetVoice().FlushSourceBuffers(), __FILE__, __LINE__);
                activity.voice->SubmitSourceBuffer(&buffer);
                ThrowIfFailed(activity.voice->GetVoice().Start(), __FILE__, __LINE__);

                auto &emitter_backend = activity.emitter_backend;
                emitter_backend.ChannelCount = 1;

                activity.state = AudioSourceActivity::State::Playing;
            } catch (...) {
                handle_exception(logger_, entity);
            }
        });
        scene_registry.remove_components<AudioPlay>(view.begin(), view.end());
    }();

    // Stop audio source when the stop command is received.
    [&]() {
        auto view = scene_registry.view<AudioSource, AudioSourceActivity, AudioStop>();
        view.each([&](SceneEntity entity, AudioSource &source, AudioSourceActivity &activity, AudioStop &stop) {
            try {
                if (activity.state == AudioSourceActivity::State::Stopped) return;

                ThrowIfFailed(activity.voice->GetVoice().Stop(), __FILE__, __LINE__);
                activity.state = AudioSourceActivity::State::Stopped;
            } catch (...) {
                handle_exception(logger_, entity);
            }
        });
        scene_registry.remove_components<AudioStop>(view.begin(), view.end());
    }();

    // Stop audio source when the buffer ends.
    scene_registry.view<AudioSource, AudioSourceActivity>()
        .each([&](SceneEntity entity, AudioSource &source, AudioSourceActivity &activity) {
            try {
                if (activity.state == AudioSourceActivity::State::Stopped) return;

                if (activity.voice->GetBufferState() == SourceVoice::BufferState::Submitting) return;

                auto position = std::chrono::duration_cast<std::chrono::milliseconds>(activity.voice->GetTimeSinceBufferStart());
                if (activity.voice->GetLoopCount() == 1) {
                    position += activity.play_begin_time;
                }

                source.position = position;

                if (activity.voice->GetBufferState() == SourceVoice::BufferState::BufferEnd) {
                    activity.state = AudioSourceActivity::State::Stopped;
                    return;
                }
            } catch (...) {
                handle_exception(logger_, entity);
            }
        });

    scene_registry.view<AudioSource, AudioSourceActivity>()
        .each([&](SceneEntity entity, AudioSource &source, AudioSourceActivity &activity) {
            if (activity.state == AudioSourceActivity::State::Stopped) return;

            activity.voice->GetVoice().SetVolume(source.volume);
        });

    [&]() {
        if (!main_listener) return;
        auto *mastering_voice = audio_platform_.mastering_voice();

        X3DAUDIO_DSP_SETTINGS dsp_settings = {};

        XAUDIO2_VOICE_DETAILS mastering_voice_details;
        mastering_voice->GetVoiceDetails(&mastering_voice_details);

        dsp_settings.DstChannelCount = mastering_voice_details.InputChannels;

        float matrix_coefficients[XAUDIO2_MAX_AUDIO_CHANNELS * 8] = {};
        dsp_settings.pMatrixCoefficients = matrix_coefficients;

        scene_registry.view<AudioSource, AudioSourceActivity>()
            .each([&](SceneEntity entity, AudioSource &source, AudioSourceActivity &activity) {
                try {
                    if (activity.state == AudioSourceActivity::State::Stopped) return;
                    if (!source.is_spatial) return;

                    dsp_settings.SrcChannelCount = activity.clip->wfx().nChannels;
                    dsp_settings.pDelayTimes = nullptr;

                    X3DAudioCalculate(audio_platform_.x3daudio_handle(), main_listener, &activity.emitter_backend,
                                      X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT,
                                      &dsp_settings);
                    activity.voice->SetOutputMatrix(mastering_voice, dsp_settings.SrcChannelCount, dsp_settings.DstChannelCount, dsp_settings.pMatrixCoefficients);
                    activity.voice->SetFrequencyRatio(dsp_settings.DopplerFactor);
                } catch (...) {
                    handle_exception(logger_, entity);
                }
            });
    }();

    [&]() {
        auto view = scene_registry.view<AudioSourceActivity>(nodec::type_list<AudioSource>{});
        scene_registry.remove_components<AudioSourceActivity>(view.begin(), view.end());
    }();
}
