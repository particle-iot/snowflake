#pragma once

#include "Particle.h"
#include "AudioPlayer.h"
#include <functional>

class VoicePulse {
    using VoicePulseCb = std::function<void()>;

public:
    VoicePulse(AudioPlayer* audioPlayer);
    ~VoicePulse();

    void registerCb(VoicePulseCb callback);

private:
    int microphone_audio_signal_get_data(size_t offset, size_t length, float* out_ptr);

private:
    AudioPlayer* audioPlayer_ = nullptr;
    int16_t* sampleBuffer_ = nullptr;
    VoicePulseCb callback_ = nullptr;
    int sliceCounter_ = 0;
    Thread* thread_ = nullptr;
};
