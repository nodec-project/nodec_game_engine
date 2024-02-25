#pragma once

#include <Exceptions.hpp>

#include <wrl/client.h>
#include <x3daudio.h>
#include <xaudio2.h>

#include <nodec/logging/logging.hpp>

#pragma comment(lib, "xaudio2.lib")

// XAudio2 Sample codes:
//  * https://github.com/walbourn/directx-sdk-samples/blob/main/XAudio2/XAudio2Sound3D/audio.cpp

inline bool operator==(const WAVEFORMATEX &left, const WAVEFORMATEX &right) {
    return (left.wFormatTag == right.wFormatTag)
           && (left.nChannels == right.nChannels)
           && (left.nSamplesPerSec == right.nSamplesPerSec)
           && (left.nBlockAlign == right.nBlockAlign)
           && (left.wBitsPerSample == right.wBitsPerSample)
           && (left.cbSize == right.cbSize);
}

inline bool operator!=(const WAVEFORMATEX &left, const WAVEFORMATEX &right) {
    return !(left == right);
}

class AudioPlatform {
public:
    AudioPlatform()
        : logger_(nodec::logging::get_logger("engine.audio-platform")) {
        using namespace Exceptions;

        ThrowIfFailed(XAudio2Create(&xaudio2_), __FILE__, __LINE__);

#if defined(_DEBUG)
        // To see the trace output, you need to view ETW logs for this application:
        //    Go to Control Panel, Administrative Tools, Event Viewer.
        //    View->Show Analytic and Debug Logs.
        //    Applications and Services Logs / Microsoft / Windows / XAudio2.
        //    Right click on Microsoft Windows XAudio2 debug logging, Properties, then Enable Logging, and hit OK
        XAUDIO2_DEBUG_CONFIGURATION debug = {};
        debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
        debug.BreakMask = XAUDIO2_LOG_ERRORS;
        xaudio2_->SetDebugConfiguration(&debug, 0);
#endif

        // Create a mastering voice
        ThrowIfFailed(xaudio2_->CreateMasteringVoice(&mastering_voice_), __FILE__, __LINE__);

        DWORD channelMask;
        ThrowIfFailed(mastering_voice_->GetChannelMask(&channelMask), __FILE__, __LINE__);

        X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, x3daudio_handle_);
    }

    ~AudioPlatform() {
        logger_->info(__FILE__, __LINE__) << "Destructed.";
    }

    IXAudio2 &xaudio() noexcept {
        return *xaudio2_.Get();
    }
    X3DAUDIO_HANDLE &x3daudio_handle() noexcept {
        return x3daudio_handle_;
    }

    IXAudio2MasteringVoice *mastering_voice() noexcept {
        return mastering_voice_;
    }

private:
    std::shared_ptr<nodec::logging::Logger> logger_;
    Microsoft::WRL::ComPtr<IXAudio2> xaudio2_;
    IXAudio2MasteringVoice *mastering_voice_{nullptr};
    X3DAUDIO_HANDLE x3daudio_handle_;
};