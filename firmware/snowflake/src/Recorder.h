#pragma once

#include "application.h"
#include "audio_hal.h"

class Recorder {
    static constexpr int SPEAKER_EN_PIN = D5;

public:
    Recorder() = default;
    ~Recorder() = default;

    int init(hal_audio_mode_t mode, hal_audio_sample_rate_t sampleRate, hal_audio_word_len_t wordLen) {
        hal_audio_init(HAL_AUDIO_OUT_DEVICE_LINEOUT, mode, sampleRate, wordLen);

        pinMode(SPEAKER_EN_PIN, OUTPUT);
        digitalWrite(SPEAKER_EN_PIN, 1);
        return 0;
    }

    int playback(void* data, size_t size) {
        hal_audio_read_dmic(data, size);
        hal_audio_write_lineout(data, size);
        return 0;
    }
};
