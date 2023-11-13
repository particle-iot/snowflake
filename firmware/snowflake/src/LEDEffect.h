#pragma once

#include <stdint.h>
#include <memory>
#include <string.h>
#include <vector>
#include <cstdlib> 
#include <random>

/// LED Effect ///


class LEDEffect {
public:
    virtual void process( uint32_t *leds, const uint32_t ledCount, const uint32_t time );
    
    //static helper that scales a colour by a brightness value
    static uint32_t ScaleColor( uint32_t color, uint8_t brightnessAsPercentage ) {
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >>  8) & 0xFF;
      uint8_t b = (color >>  0) & 0xFF;
      
      r = (r * brightnessAsPercentage) / 100;
      g = (g * brightnessAsPercentage) / 100;
      b = (b * brightnessAsPercentage) / 100;
      
      return MakeColor(r, g, b);
    }

    static uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b) {
      return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
    }

    static uint32_t MakeColorScaled(uint8_t r, uint8_t g, uint8_t b, const uint8_t brightnessAsPercentage ) {
      r = (r * brightnessAsPercentage) / 100;
      g = (g * brightnessAsPercentage) / 100;
      b = (b * brightnessAsPercentage) / 100;
      
      return MakeColor(r, g, b);
    }

    static uint32_t random( const uint32_t min, const uint32_t max ) {
      return min + (rand() % (int)(max - min + 1));
    }

    static std::vector<uint8_t> createEvenlyDistributedLEDFlashSequenceVector(const uint8_t totalLEDs ) {
        std::vector<uint8_t> flashSequence(totalLEDs, 0); // Initialize all LEDs as off (0)
        std::random_device rd;
        std::mt19937 gen(rd());
        
        for (int i = 0; i < totalLEDs; ++i) {
            uint8_t position = 0;
            do {
                std::uniform_int_distribution<> distrib(0, totalLEDs - 1);
                position = distrib(gen);
            } while (flashSequence[position] != 0); // Ensure unique position
            
            flashSequence[position] = 1; // Set LED to flash (1)
        }

        return flashSequence;
    }

    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    static uint32_t colourWheel(uint8_t wheelPos) {
        if (wheelPos < 85) {
            return LEDEffect::MakeColor(wheelPos * 3, 255 - wheelPos * 3, 0);
        } else if (wheelPos < 170) {
            wheelPos -= 85;
            return LEDEffect::MakeColor(255 - wheelPos * 3, 0, wheelPos * 3);
        } else {
            wheelPos -= 170;
            return LEDEffect::MakeColor(0, wheelPos * 3, 255 - wheelPos * 3);
        }
    }
};

//// Pixel Providers ////

class LEDPixelProvider {
    public:
        virtual std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount );
};


class AllPixelsProvider : public LEDPixelProvider {
    public:
        std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount ) {
            pixelCount = 36;
            uint8_t* allPixelsMem = new uint8_t[pixelCount];

            for( uint8_t i = 0; i < pixelCount; i++ ) {
                allPixelsMem[i] = i;
            }

            return std::unique_ptr<uint8_t[]>(allPixelsMem);
        }
};


class InnerCirclePixelProvider : public LEDPixelProvider {
    public:
        std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount ) {

            //get the circle LEDS and return them. there are 12 in total
            const uint8_t circleLEDs[] = { 0, 5, 6, 11, 12, 17, 18, 23, 24, 29, 30, 35 };

            uint8_t* circleLEDsMem = new uint8_t[sizeof(circleLEDs)];
            memcpy(circleLEDsMem, circleLEDs, sizeof(circleLEDs));

            pixelCount = 12;
            return std::unique_ptr<uint8_t[]>(circleLEDsMem);
        }
};



class EveryNPixelProvider : public LEDPixelProvider {
    public:
        EveryNPixelProvider( const uint8_t n ) : n_(n) {};

        std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount ) {

            //inner loop speed
            #define ROTATION_SPEED 330

            //which led should we start on?
            const uint8_t startLed = ((timeInMS / ROTATION_SPEED) % n_);

            //calculate how many total LEDs will we have
            pixelCount = (36 - startLed) / n_;

            uint8_t* everyNMem = new uint8_t[pixelCount];

            //fill in the everyNMem
            for( uint8_t i = 0; i < pixelCount; i += 1 ) {
                everyNMem[i] = startLed + (i * n_);
            }

            return std::unique_ptr<uint8_t[]>(everyNMem);
        }
    
    private:
        uint8_t n_;
};


class PetalPixelProvider : public LEDPixelProvider {
    public:
        PetalPixelProvider( const bool includeStem, const bool includeRoots, const uint32_t petalChangeTimeInMS )
            : includeStem_(includeStem), includeRoots_(includeRoots), petalChangeTimeInMS_(petalChangeTimeInMS) {

            //define the petal order
            //there are 6 petals in total
            petalOrder_[0] = 1;
            petalOrder_[1] = 4;
            petalOrder_[2] = 0;
            petalOrder_[3] = 3;
            petalOrder_[4] = 5;
            petalOrder_[5] = 2;
        };

        std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount ) {

            //we provide a different petal every 3 seconds
            const uint8_t petalIndex = petalOrder_[((timeInMS / petalChangeTimeInMS_) % 6)];

            if( includeStem_ )
            {
                if( includeRoots_ )
                {
                    //there are 6 petals in total
                    const uint8_t petal1[] = { 35, 0, 1, 2, 3, 4, 5 };
                    const uint8_t petal2[] = { 5, 6, 7, 8, 9, 10, 11 };
                    const uint8_t petal3[] = { 11, 12, 13, 14, 15, 16, 17 };
                    const uint8_t petal4[] = { 17, 18, 19, 20, 21, 22, 23 };
                    const uint8_t petal5[] = { 23, 24, 25, 26, 27, 28, 29 };
                    const uint8_t petal6[] = { 29, 30, 31, 32, 33, 34, 35 };

                    //get the petal we want
                    const uint8_t *petalArrays[6] = { petal1, petal2, petal3, petal4, petal5, petal6 };

                    uint8_t* petalLEDMem = new uint8_t[sizeof(petal1)];
                    memcpy(petalLEDMem, petalArrays[petalIndex], sizeof(petal1));

                    pixelCount = sizeof(petal1);
                    return std::unique_ptr<uint8_t[]>(petalLEDMem);
                }
                else
                {
                    //there are 6 petals in total
                    const uint8_t petal1[] = { 0, 1, 2, 3, 4 };
                    const uint8_t petal2[] = { 6, 7, 8, 9, 10 };
                    const uint8_t petal3[] = { 12, 13, 14, 15, 16 };
                    const uint8_t petal4[] = { 18, 19, 20, 21, 22 };
                    const uint8_t petal5[] = { 24, 25, 26, 27, 28 };
                    const uint8_t petal6[] = { 30, 31, 32, 33, 34 };

                    //get the petal we want
                    const uint8_t *petalArrays[6] = { petal1, petal2, petal3, petal4, petal5, petal6 };

                    uint8_t* petalLEDMem = new uint8_t[sizeof(petal1)];
                    memcpy(petalLEDMem, petalArrays[petalIndex], sizeof(petal1));

                    pixelCount = sizeof(petal1);
                    return std::unique_ptr<uint8_t[]>(petalLEDMem);
                }
            }
            else
            {
                //there are 6 petals in total
                const uint8_t petal1[] = { 1, 2, 3, 4 };
                const uint8_t petal2[] = { 7, 8, 9, 10 };
                const uint8_t petal3[] = { 13, 14, 15, 16 };
                const uint8_t petal4[] = { 19, 20, 21, 22 };
                const uint8_t petal5[] = { 25, 26, 27, 28 };
                const uint8_t petal6[] = { 31, 32, 33, 34 };

                //get the petal we want
                const uint8_t *petalArrays[6] = { petal1, petal2, petal3, petal4, petal5, petal6 };

                uint8_t* petalLEDMem = new uint8_t[sizeof(petal1)];
                memcpy(petalLEDMem, petalArrays[petalIndex], sizeof(petal1));

                pixelCount = sizeof(petal1);
                return std::unique_ptr<uint8_t[]>(petalLEDMem);
            }
        }

    private:
        bool includeStem_;      
        bool includeRoots_;
        uint32_t petalChangeTimeInMS_;
        uint8_t petalOrder_[6];
};



//// Color Providers ////

class LEDColorProvider {
    public:
        virtual std::unique_ptr<uint32_t[]> getColours( const uint32_t numberPixels, const uint32_t timeInMS );
};


class RainbowColorProvider : public LEDColorProvider {
    public:
        std::unique_ptr<uint32_t[]> getColours( const uint32_t numberPixels, const uint32_t timeInMS );
};


class FixedColorProvider : public LEDColorProvider {
    public:
        FixedColorProvider( const uint32_t color ) : color_(color) {};

        std::unique_ptr<uint32_t[]> getColours( const uint32_t numberPixels, const uint32_t timeInMS );

    private:
        uint32_t color_;
};


class ChaseColorProvider : public LEDColorProvider {
    public:
        ChaseColorProvider( const uint32_t color, const uint8_t chaseSize ) : color_(color), chaseSize_(chaseSize) {};

        std::unique_ptr<uint32_t[]> getColours( const uint32_t numberPixels, const uint32_t timeInMS );

    private:
        uint32_t color_;
        uint8_t chaseSize_;
};


class GlowColorProvider : public LEDColorProvider {
    public:
        GlowColorProvider( const uint32_t baseColor, const uint32_t glowColor, const uint32_t glowTimeWindow )
            : base_color_(baseColor), glow_color_(glowColor), glowTimeWindow_(glowTimeWindow) {};

        std::unique_ptr<uint32_t[]> getColours( const uint32_t numberPixels, const uint32_t timeInMS );

    private:
        uint32_t base_color_;
        uint32_t glow_color_;
        uint32_t glowTimeWindow_;
};


//This effect takes a pixel and sparkles it to the fix colour and then back to the base colour again
// It blends itself over the base color
class SparkleLED
{
    public:
      SparkleLED( const uint32_t led, const uint32_t startTime ) : led_(led), startTime_(startTime) {};

      uint32_t led_;
      uint32_t startTime_;
};

class SparkleColorProvider : public LEDColorProvider {
    public:
        SparkleColorProvider( const uint32_t color, const uint32_t numberOfSparkles, const uint32_t sparkleTime, const uint32_t sparkleSpreadTime )
            : color_(color), numberOfSparkles_(numberOfSparkles), sparkleTime_(sparkleTime), sparkleSpreadTime_(sparkleSpreadTime) {

            //This crashes for some reason - to be debugged!
            //flashSequence_ = LEDEffect::createEvenlyDistributedLEDFlashSequenceVector(36);

            //I hard coded this instead
            uint8_t ledSequence[] = {28, 12, 21, 20, 19, 35, 23, 7, 10, 3, 17, 1, 18, 13, 6, 33, 15, 32, 0, 26, 11, 16, 27, 24, 34, 30, 9, 4, 14, 8, 5, 31, 25, 29, 22, 2};

            // Initialize vector in a single line
            flashSequence_ = std::vector<uint8_t>(std::begin(ledSequence), std::end(ledSequence));

            //create 5 sparkle LEDS. Each LED number is from the spread flash sequence and the start time is randomized from now up to 10 seconds later
            for( uint8_t i = 0; i < numberOfSparkles_; i++ ) {
                sparkleLEDs_.push_back( SparkleLED( flashSequence_[flashSequenceIndex_], LEDEffect::random(0, sparkleSpreadTime_) ) );
                incFlashSequenceIndex();
            }
        };

        std::unique_ptr<uint32_t[]> getColours( const uint32_t numberPixels, const uint32_t timeInMS );

    private:
        void incFlashSequenceIndex() {
            flashSequenceIndex_++;
            if( flashSequenceIndex_ >= flashSequence_.size() ) {
                flashSequenceIndex_ = 0;
            }
        }

        uint32_t color_;
        uint32_t numberOfSparkles_;
        uint32_t sparkleTime_;
        uint32_t sparkleSpreadTime_;

        //a vector to hold the output of createEvenlyDistributedLEDFlashSequenceVector
        std::vector<uint8_t> flashSequence_;
        uint8_t flashSequenceIndex_;

        //a dynamic array of SparkleLED
        std::vector<SparkleLED> sparkleLEDs_;
};


/// LED effect implementations ///

class LEDEffectPixelAndColor : LEDEffect {
  public:
    LEDEffectPixelAndColor( LEDPixelProvider &pixelProvider, LEDColorProvider &colorProvider )
      : pixelProvider_(pixelProvider), colorProvider_(colorProvider) {};

    void process( uint32_t *leds, const uint32_t ledCount, const uint32_t time );

  private:
    LEDPixelProvider &pixelProvider_;
    LEDColorProvider &colorProvider_;
};
