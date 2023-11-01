#pragma once

#include "application.h"
#include "audio_hal.h"
#include <fcntl.h>

class MusicPlayer {
    static constexpr int SPEAKER_EN_PIN = D5;

public:
    MusicPlayer() = default;
    ~MusicPlayer() = default;

    // init with audio settings
    void init(hal_audio_mode_t mode, hal_audio_sample_rate_t sampleRate, hal_audio_word_len_t wordLen) {
        hal_audio_init(HAL_AUDIO_OUT_DEVICE_LINEOUT, mode, sampleRate, wordLen);

        pinMode(SPEAKER_EN_PIN, OUTPUT);
        digitalWrite(SPEAKER_EN_PIN, 1);
    }

    void deinit() {
        digitalWrite(SPEAKER_EN_PIN, 0);
        pinMode(SPEAKER_EN_PIN, PIN_MODE_NONE);
        hal_audio_deinit();
    }

    int playFile(const char* fileName) {
        int fd = open(fileName, O_RDONLY);
        CHECK_TRUE_RETURN(fd >= 0, SYSTEM_ERROR_FILESYSTEM);

        static constexpr int WAV_BUF_SIZE = 1024;
        std::unique_ptr<uint8_t[]> data(new uint8_t[WAV_BUF_SIZE]);

        struct stat fileStat = {};
        stat(fileName, &fileStat);

        for (auto i = 0; i < fileStat.st_size; i += WAV_BUF_SIZE) {
            auto readSize = read(fd, data.get(), WAV_BUF_SIZE);
            if (readSize <= 0) {
                break;
            }
            hal_audio_write_lineout(data.get(), WAV_BUF_SIZE);
        }
        close(fd);

        return 0;
    }

    int playBuffer(uint8_t* buffer, size_t size) {
        hal_audio_write_lineout(buffer, size);
        return 0;
    }
};
