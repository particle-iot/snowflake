#include "RgbStrip.h"

#define PIXEL_PIN SPI
#define PIXEL_COUNT 36
#define PIXEL_TYPE WS2812B

RgbStrip::RgbStrip() {
    strip_ = new Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
    thread_ = new Thread("rgbThread", [this]()->os_thread_return_t{
        strip_->begin();
        strip_->setBrightness(50);
        strip_->show();
        while (1) {
            rainbow(20);
        }
    });
}

RgbStrip::~RgbStrip() {
    delete strip_;
    delete thread_;
}

void RgbStrip::showColor(uint8_t r, uint8_t g, uint8_t b) {
    strip_->setBrightness(128);
    strip_->begin();
    for (int i = 0; i < strip_->numPixels(); i++) {
        strip_->setPixelColor(i, 255, 255, 255);
    }
    strip_->show();
}

void RgbStrip::rainbow(uint8_t wait) {
    for (uint16_t j = 0; j < 256; j++) {
        for (uint16_t i = 0; i < strip_->numPixels(); i++) {
            strip_->setPixelColor(i, wheel((i + j) & 255));
        }
        strip_->show();
        delay(wait);
    }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t RgbStrip::wheel(byte wheelPos) {
    if (wheelPos < 85) {
        return strip_->Color(wheelPos * 3, 255 - wheelPos * 3, 0);
    } else if (wheelPos < 170) {
        wheelPos -= 85;
        return strip_->Color(255 - wheelPos * 3, 0, wheelPos * 3);
    } else {
        wheelPos -= 170;
        return strip_->Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
}
