#pragma once

#include "AudioPlatform.hpp"
#include <Exceptions.hpp>

#include <atomic>
#include <chrono>
#include <thread>

class SourceVoice : private IXAudio2VoiceCallback {
public:
    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;
    using duration = clock::duration;

    enum class BufferState {
        Submitting,
        BufferStart,
        BufferEnd
    };

public:
    SourceVoice(AudioPlatform *pAudioPlatform, const WAVEFORMATEX &wfx)
        : mWfx{wfx} {
        using namespace Exceptions;

        nodec::logging::InfoStream(__FILE__, __LINE__) << std::this_thread::get_id();

        mX3DAudioHandle = &pAudioPlatform->x3daudio_handle();

        ThrowIfFailed(
            pAudioPlatform->xaudio().CreateSourceVoice(
                &mpSourceVoice, &wfx,
                0, 2.0, this),
            __FILE__, __LINE__);
    }

    ~SourceVoice() {
        if (mpSourceVoice) {
            mpSourceVoice->Stop();
            mpSourceVoice->DestroyVoice();
            mpSourceVoice = nullptr;
        }
    }

    IXAudio2SourceVoice &GetVoice() {
        return *mpSourceVoice;
    }

    WAVEFORMATEX &GetWfx() {
        return mWfx;
    }

    void SubmitSourceBuffer(const XAUDIO2_BUFFER *buffer) {
        using namespace Exceptions;
        mBufferState = BufferState::Submitting;
        ThrowIfFailed(mpSourceVoice->SubmitSourceBuffer(buffer), __FILE__, __LINE__);
    }

    // setting X3DAudio Method
    // TODO: state追加必要？要検討
    X3DAUDIO_HANDLE *GetX3DAudioHandle() noexcept {
        return mX3DAudioHandle;
    }

    void SetFrequencyRatio(FLOAT32 dopplerFactor) {
        using namespace Exceptions;  
        ThrowIfFailed(mpSourceVoice->SetFrequencyRatio(dopplerFactor), __FILE__, __LINE__);
    }

    void SetOutputMatrix(IXAudio2Voice *pDestinationVoice, UINT32 sourceChannels, UINT32 destinationChannels, const float *pLevelMatrix, UINT32 operationSet = 0U) {
        using namespace Exceptions;
        ThrowIfFailed(mpSourceVoice->SetOutputMatrix(pDestinationVoice, sourceChannels, destinationChannels, pLevelMatrix, operationSet), __FILE__, __LINE__);
    }

    void SetFilterParameters(const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 operationSet = 0U) {
        using namespace Exceptions;
        ThrowIfFailed(mpSourceVoice->SetFilterParameters(pParameters, operationSet), __FILE__, __LINE__);
    }

    BufferState GetBufferState() const {
        return mBufferState;
    }

    int GetLoopCount() const {
        return mLoopCount;
    }

    duration GetTimeSinceBufferStart() const {
        return clock::now() - mBufferStartTime.load();
    }

private:
    void OnBufferStart(void *) override {
        using namespace nodec;
        logging::InfoStream(__FILE__, __LINE__) << "[OnBufferStart]" << std::this_thread::get_id();
        mBufferStartTime = clock::now();
        mLoopCount = 1;
        mBufferState = BufferState::BufferStart;
    }

    void OnBufferEnd(void *) override {
        using namespace nodec;
        logging::InfoStream(__FILE__, __LINE__) << "[OnBufferEnd]" << std::this_thread::get_id();
        mBufferState = BufferState::BufferEnd;
    }

    void OnLoopEnd(void *) override {
        using namespace nodec;
        mBufferStartTime = clock::now();
        ++mLoopCount;
        logging::InfoStream(__FILE__, __LINE__) << "[OnLoopEnd]" << std::this_thread::get_id();
    }

    void OnStreamEnd() override {
        using namespace nodec;
        logging::InfoStream(__FILE__, __LINE__) << "[OnStreamEnd]" << std::this_thread::get_id();
    }

    void OnVoiceError(void *, HRESULT) override {
        using namespace nodec;
        logging::InfoStream(__FILE__, __LINE__) << "[OnStreamEnd]" << std::this_thread::get_id();
    }

    void OnVoiceProcessingPassStart(UINT32) override {
        using namespace nodec;
        // logging::InfoStream(__FILE__, __LINE__) << "[OnVoiceProcessingPassStart]" << std::this_thread::get_id();
    }
    void OnVoiceProcessingPassEnd() override {
        using namespace nodec;
        // logging::InfoStream(__FILE__, __LINE__) << "[OnVoiceProcessingPassEnd]" << std::this_thread::get_id();
    }

private:
    IXAudio2SourceVoice *mpSourceVoice{nullptr};
    X3DAUDIO_HANDLE *mX3DAudioHandle;

    WAVEFORMATEX mWfx;

    std::atomic<BufferState> mBufferState;
    std::atomic<time_point> mBufferStartTime;
    std::atomic<int> mLoopCount{1};
};