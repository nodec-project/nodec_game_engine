#include <SceneAudio/SceneAudioSystem.hpp>
#include <nodec_scene/components/local_to_world.hpp>

void SceneAudioSystem::UpdateAudio(nodec_scene::SceneRegistry &registry) {
    using namespace nodec;
    using namespace nodec_scene_audio::components;
    using namespace nodec_scene::components;
    using namespace Exceptions;
    X3DAUDIO_LISTENER listener = {};
    auto calcVec = [](nodec::Matrix4x4f maxrix, nodec::Vector4f vec) {
        nodec::Vector4f temp = maxrix * vec;
        return X3DAUDIO_VECTOR{temp.x, temp.y, temp.z};
    };

    // TODO: ここの座標コンポーネントはローカル座標なので、グローバルに変換するためのLocalToWorldコンポーネントがいる
    registry.view<AudioListener, LocalToWorld>().each([&](auto entt, AudioListener &source, LocalToWorld &trfm) {
        nodec::Matrix4x4f localTramsform = trfm.value;
        listener.OrientFront = calcVec(localTramsform, {0, 0, 1, 1});
        listener.OrientTop = calcVec(localTramsform, {0, 1, 0, 1});
        listener.Position = calcVec(localTramsform, {0, 0, 0, 1});
        // NOTE: 計算で出す.前回フレームとの差分
        // TODO: EmitterとListenerコンポーネントに、velocityのprivateコンポーネントを持たせる
        listener.Velocity = {0, 0, 0};
    });

    // update entities with AudioSource.
    registry.view<AudioSource, LocalToWorld, AudioSourceActivity>().each([&](auto entt, AudioSource &source, LocalToWorld &trfm, AudioSourceActivity &activity) {
        try {
            switch (activity.state) {
            case AudioSourceActivity::State::Stopped: {
                if (!source.is_playing) break;

                // -> Playing

                // Casts the client clip to backend clip and the activity owns it.
                activity.pClip = std::static_pointer_cast<AudioClipBackend>(source.clip);

                if (!activity.pClip) break;

                if (!activity.pVoice || activity.pVoice->GetWfx() != activity.pClip->wfx()) {
                    activity.pVoice.reset(new SourceVoice(audio_platform_, activity.pClip->wfx()));
                }

                const auto &wfx = activity.pClip->wfx();
                int playBegin = source.position.count() * wfx.nSamplesPerSec / 1000;
                int totalSampleFrames = activity.pClip->samples().size() / wfx.nBlockAlign;

                // logging::InfoStream(__FILE__, __LINE__) << "*** " << playBegin << " : " << totalSampleFrames << " , " << wfx.nSamplesPerSec
                //                                         << ", " << wfx.wBitsPerSample;

                if (playBegin < 0 || totalSampleFrames <= playBegin) break;

                XAUDIO2_BUFFER buffer = {};
                buffer.pAudioData = activity.pClip->samples().data();
                buffer.Flags = XAUDIO2_END_OF_STREAM;
                buffer.AudioBytes = activity.pClip->samples().size();
                buffer.LoopCount = source.loop ? XAUDIO2_LOOP_INFINITE : 0;

                buffer.PlayBegin = playBegin;
                activity.playBeginTime = source.position;

                // setting X3DAudio option

                X3DAUDIO_EMITTER emitter = {};
                // TODO: set Listener, Emitter pos
                nodec::Matrix4x4f localTramsform = trfm.value;
                emitter.OrientFront = calcVec(localTramsform, {0, 0, 1, 0});
                emitter.OrientTop = calcVec(localTramsform, {0, 1, 0, 0});
                emitter.Position = calcVec(localTramsform, {0, 0, 0, 1});
                emitter.Velocity = {0, 0, 0};

                IXAudio2MasteringVoice *MasterVoice = audio_platform_->mastering_voice();
                XAUDIO2_VOICE_DETAILS DeviceDetails;
                MasterVoice->GetVoiceDetails(&DeviceDetails);
                // TODO: AudioClipで定義してあるMono/Stereo情報をどうやってとるのか？
                UINT32 SourceChannels = source.clip->channelCount;
                // NOTE: これでいいかの確証がないので調べる
                UINT32 DestinationChannels = DeviceDetails.InputChannels;
                X3DAUDIO_DSP_SETTINGS DSPSettings = {};
                // TODO: setting X3DAUDIO_DSP_SETTINGS param

                // NOTE: 以下4つのパラメータは、X3DAudioCalculateを行う前に初期化する必要あり
                DSPSettings.SrcChannelCount = SourceChannels;
                // NOTE: SrcChannelCountと何が違うのか?
                DSPSettings.DstChannelCount = DestinationChannels;
                DSPSettings.pMatrixCoefficients = new FLOAT32[DestinationChannels];
                // TODO: 後々プロパティ化する
                DSPSettings.pDelayTimes = nullptr;

                X3DAUDIO_HANDLE& X3DInstance = audio_platform_->x3daudio_handle();

                X3DAudioCalculate(X3DInstance, &listener, &emitter,
                                  X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT,
                                  &DSPSettings);

                activity.pVoice->SetOutputMatrix(MasterVoice, SourceChannels, DestinationChannels, DSPSettings.pMatrixCoefficients);
                activity.pVoice->SetFrequencyRatio(DSPSettings.DopplerFactor);

                // TODO: pSubmixVoiceの取得方法
                // TODO: reverbは後々対応
                // activity.pVoice->SetOutputMatrix(pSubmixVoice, SourceChannels, DestinationChannels, &DSPSettings.ReverbLevel);

                // ローパスフィルタ適応
                // NOTE: voiceが対応していないという旨のエラーが出る
                // XAudio2: Filter control is not available on this voice
                // XAUDIO2_FILTER_PARAMETERS FilterParameters = {XAUDIO2_FILTER_TYPE::LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * DSPSettings.LPFDirectCoefficient), 1.0f};
                // activity.pVoice->SetFilterParameters(&FilterParameters);

                ThrowIfFailed(activity.pVoice->GetVoice().FlushSourceBuffers(), __FILE__, __LINE__);
                activity.pVoice->SubmitSourceBuffer(&buffer);
                // ThrowIfFailed(activity.pVoice->GetVoice().SubmitSourceBuffer(&buffer), __FILE__, __LINE__);
                ThrowIfFailed(activity.pVoice->GetVoice().Start(), __FILE__, __LINE__);

                activity.state = AudioSourceActivity::State::Playing;

            } break;

            case AudioSourceActivity::State::Playing: {
                using namespace std::chrono;

                if (activity.pVoice->GetBufferState() == SourceVoice::BufferState::Submitting) break;

                auto position = duration_cast<milliseconds>(activity.pVoice->GetTimeSinceBufferStart());
                if (activity.pVoice->GetLoopCount() == 1) {
                    position += activity.playBeginTime;
                }

                source.position = position;

                if (activity.pVoice->GetBufferState() == SourceVoice::BufferState::BufferEnd) {
                    // -> Stopped
                    source.is_playing = false;
                    activity.state = AudioSourceActivity::State::Stopped;
                    break;
                }

                if (!source.is_playing) {
                    // -> Stopped
                    ThrowIfFailed(activity.pVoice->GetVoice().Stop(), __FILE__, __LINE__);
                    activity.state = AudioSourceActivity::State::Stopped;
                    break;
                }
            } break;
            }
        } catch (...) {
            [&]() {
                logging::ErrorStream error(__FILE__, __LINE__);
                error << "[SceneAudioSystem::UpdateAudio] >>> Audio Error. \n"
                         "entity: "
                      << std::hex << "0x" << entt << std::dec << "\n"
                                                                 "details:\n";
                try {
                    throw;
                } catch (std::exception &e) {
                    error << e.what();
                } catch (...) {
                    error << "Unknown";
                }
            }();
        }
    });

    // TODO: Update entities with AudioListener.
}
