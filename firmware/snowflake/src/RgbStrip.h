#pragma once

#include "application.h"
#include "neopixel.h"
#include <stdint.h>

class RgbStrip {
public:
    RgbStrip();
    ~RgbStrip();

    //a enum of modes
    // - main snowflake mode
    // - hanukka snowflake
    // - rainbow snowflake
    // - chase snowflake

    enum MODES_T {
        MODE_OFF,
        MODE_SNOWFLAKE,
        MODE_HANUKKAH,
        MODE_RAINBOW,
        MODE_CHASE_HOLIDAY,
        MODE_CIRCLES_ROTATE,

        MODE_MAX
    };

    void setMode( const MODES_T mode ) {
        if( mode < MODE_MAX ) 
        {
            mode_ = mode;
        }
        else
        {
            Log.info( "Invalid mode: %d", mode );
        }
    }

    void showColor(uint8_t r, uint8_t g, uint8_t b);
    void rainbow(uint8_t wait);

    void set_pixels(const uint32_t* pixels, const uint8_t brightness, size_t num_pixels);

    void colorWipe(uint32_t color, int wait);
    void theaterChase(uint32_t color, int wait);

private:
    uint32_t wheel(byte wheelPos);

    MODES_T mode_;

private:
    Adafruit_NeoPixel* strip_;
    Thread* thread_;
};
