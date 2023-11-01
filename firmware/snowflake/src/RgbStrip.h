#pragma once

#include "application.h"
#include "neopixel.h"
#include <stdint.h>

class RgbStrip {
public:
    RgbStrip();
    ~RgbStrip();

    void showColor(uint8_t r, uint8_t g, uint8_t b);
    void rainbow(uint8_t wait);

private:
    uint32_t wheel(byte wheelPos);

private:
    Adafruit_NeoPixel* strip_;
    Thread* thread_;
};



