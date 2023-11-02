#include "Particle.h"
#include "FileReceiver.h"
#include "MusicPlayer.h"
#include "Recorder.h"
#include "FileSystemWrapper.h"
#include "RgbStrip.h"
#include "NtcThermistor.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

SYSTEM_MODE(MANUAL);

Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    {"app", LOG_LEVEL_ALL}
});

void Demo_00_MusicFileReceiver() {
    // removeAllFiles("/");
    dumpFilesAndDirs("/");

    RGB.control(true);
    RGB.color(255, 255, 255); // state idle : white

    static volatile bool buttonClicked = false;
    System.on(button_click, [](system_event_t ev, int button_data){
        buttonClicked = true;
    });

    while (1) {
        if (buttonClicked) {
            buttonClicked = false;
            RGB.color(0, 255, 0); // state transter on-going : green
            int ret = fileReceiver();
            if (ret) {
                Log.error("fileReceiver failed, ret: %d", ret);
                RGB.color(255, 0, 0); // state error: red
            } else {
                Log.info("fileReceiver success");
                RGB.color(0, 255, 255); // state success: cyan
            }
        }
    }
}

void Demo_01_MusicPlayer() {
    MusicPlayer musicPlayer;
    musicPlayer.init(HAL_AUDIO_MODE_STEREO, HAL_AUDIO_SAMPLE_RATE_16K, HAL_AUDIO_WORD_LEN_16);
    musicPlayer.playFile("let_it_snow_20s_16k.wav");
}

void Demo_02_Recorder() {
    Recorder recorder;
    recorder.init(HAL_AUDIO_MODE_STEREO, HAL_AUDIO_SAMPLE_RATE_16K, HAL_AUDIO_WORD_LEN_16);

    static volatile bool buttonClicked = false;
    System.on(button_click, [](system_event_t ev, int button_data){
        buttonClicked = true;
    });

    RGB.control(true);
    RGB.color(0, 255, 0); // Green means idle state

    size_t voiceDataSize = 16 * 1024 * 15; // 15 seconds with 16K sample rate
    void* voiceData = malloc(voiceDataSize);

    while (1) {
        if (buttonClicked) {
            buttonClicked = false;

            RGB.color(255, 0, 0); // red
            hal_audio_read_dmic(voiceData, voiceDataSize);
            RGB.color(0, 0, 255); // blue
            delay(3000);
            hal_audio_write_lineout(voiceData, voiceDataSize);

            RGB.color(0, 255, 0); // green
        }
    }
}

void Demo_03_TouchToGetTemperature() {
    static constexpr int TOUCH_PIN = D10;
    static volatile bool buttonClicked = false;

    attachInterrupt(TOUCH_PIN, +[](){
        buttonClicked = true;
    }, FALLING);

    while (1) {
        if (buttonClicked) {
            buttonClicked = false;
            float temperature = ntcGetTemperature();
            Log.info("Temperature: %.1f", temperature);
        }
    }
}

void setup() {
    RgbStrip rgbStrip;

    // Demo_00_MusicFileReceiver();
    Demo_01_MusicPlayer();
    // Demo_02_Recorder();
    // Demo_03_TouchToGetTemperature();
}

void loop() {

}

