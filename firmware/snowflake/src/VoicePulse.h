#pragma once

#include "Particle.h"
#include "AudioPlayer.h"
#include <functional>

class VoicePulse {
    using VoicePulseDetectedCb = std::function<void()>;

public:
    VoicePulse(AudioPlayer* audioPlayer, VoicePulseDetectedCb callback, const float threshold = 0.5f);

    void start( void );

private:
    int microphone_audio_signal_get_data(size_t offset, size_t length, float* out_ptr);

private:
    AudioPlayer* audioPlayer_ = nullptr;
    int16_t* sampleBuffer_ = nullptr;
    VoicePulseDetectedCb callback_ = nullptr;
    int sliceCounter_ = 0;
    float threshold_ = 0;
    Thread* thread_ = nullptr;
};
