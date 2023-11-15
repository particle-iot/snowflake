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

    static const uint32_t TRANSPARENT_COLOR = 0xFF000000;
    
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

//These are the LEDs around the center loop
class InnerCirclePixelProvider : public LEDPixelProvider {
    public:
        std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount ) {

            //get the circle LEDS and return them. there are 12 in total
            const uint8_t circleLEDs[] = { 0, 5, 6, 11, 12, 17, 18, 23, 24, 29, 30, 35 };

            uint8_t* circleLEDsMem = new uint8_t[sizeof(circleLEDs)];
            memcpy(circleLEDsMem, circleLEDs, sizeof(circleLEDs));

            pixelCount = sizeof(circleLEDs);
            return std::unique_ptr<uint8_t[]>(circleLEDsMem);
        }
};


//these are the LEDs outside of the center loop - they don't quite make a perfect loop
class OuterCirclePixelProvider : public LEDPixelProvider {
    public:
        std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount ) {

            //get the circle LEDS and return them. there are 12 in total
            const uint8_t circleLEDs[] = { 1, 2, 4, 7, 8, 10, 13, 14, 16, 19, 20, 22, 25, 26, 28, 31, 32, 34 };

            uint8_t* circleLEDsMem = new uint8_t[sizeof(circleLEDs)];
            memcpy(circleLEDsMem, circleLEDs, sizeof(circleLEDs));

            pixelCount = sizeof(circleLEDs);
            return std::unique_ptr<uint8_t[]>(circleLEDsMem);
        }
};



//this simulates a clock hand going around the snowflake
class ClockHandPixelProvider : public LEDPixelProvider {
    public:

        typedef struct
        {
            uint8_t ledCount;
            uint8_t leds[];
        } HAND_POSITION_T;

        std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount ) {

            // const HAND_POSITION_T handPosition[] = {
            //     { 4, { 0, 2, 3 } }
            // };

            //get the circle LEDS and return them. there are 12 in total
            const uint8_t circleLEDs[] = { 1, 2, 4, 7, 8, 10, 13, 14, 16, 19, 20, 22, 25, 26, 28, 31, 32, 34 };

            uint8_t* circleLEDsMem = new uint8_t[sizeof(circleLEDs)];
            memcpy(circleLEDsMem, circleLEDs, sizeof(circleLEDs));

            pixelCount = sizeof(circleLEDs);
            return std::unique_ptr<uint8_t[]>(circleLEDsMem);
        }
};



class EveryNPixelProvider : public LEDPixelProvider {
    public:
        EveryNPixelProvider( const uint8_t n, const uint32_t offset ) : n_(n), offset_(offset) {};

        std::unique_ptr<uint8_t[]> getPixels( const uint32_t timeInMS, uint8_t &pixelCount ) {

            //inner loop speed
            #define ROTATION_SPEED 330*2

            const uint32_t timeInMSOffset = timeInMS + ((ROTATION_SPEED/n_) * offset_);

            //which led should we start on?
            const uint8_t startLed = ((timeInMSOffset / ROTATION_SPEED) % n_);

            //calculate how many total LEDs will we have
            pixelCount = (36 / n_);

            uint8_t* everyNMem = new uint8_t[pixelCount];

            //fill in the everyNMem
            for( uint8_t i = 0; i < pixelCount; i += 1 ) {
                everyNMem[i] = startLed + (i * n_);
            }

            return std::unique_ptr<uint8_t[]>(everyNMem);
        }
    
    private:
        uint8_t n_;
        uint32_t offset_;
};


#if 1 //NOTE - rewrite below. To finish!
class PetalPixelProvider : public LEDPixelProvider {
    public:

        typedef enum
        {
            PETAL_JUST_TIP,
            PETAL_NORMAL,
            PETAL_STEM,
            PETAL_ROOTS

        } PETAL_TYPE_T;

        typedef enum
        {
            PETAL_MOVEMENT_ALL_ON,
            PETAL_MOVEMENT_ROTATE,

        } PETAL_MOVEMENT_T;

        PetalPixelProvider( const PETAL_TYPE_T petalType, const PETAL_MOVEMENT_T movement, const uint32_t petalChangeTimeInMS )
            : petalType_(petalType), petalMovement_(movement), petalChangeTimeInMS_(petalChangeTimeInMS) {

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

            switch( petalType_ )
            {
                case PETAL_JUST_TIP:
                {
                    //there are 6 petals in total
                    const uint8_t petal1[] = { 3 };
                    const uint8_t petal2[] = { 9 };
                    const uint8_t petal3[] = { 15 };
                    const uint8_t petal4[] = { 21 };
                    const uint8_t petal5[] = { 27 };
                    const uint8_t petal6[] = { 33 };

                    //get the petal we want
                    const uint8_t *petalArrays[6] = { petal1, petal2, petal3, petal4, petal5, petal6 };

                    if( petalMovement_ == PETAL_MOVEMENT_ROTATE )
                    {
                        uint8_t* petalLEDMem = new uint8_t[sizeof(petal1)];
                        memcpy(petalLEDMem, petalArrays[petalIndex], sizeof(petal1));

                        pixelCount = sizeof(petal1);
                        return std::unique_ptr<uint8_t[]>(petalLEDMem);
                    }
                    else
                    {
                        //all on!
                        pixelCount = sizeof(petal1) + sizeof(petal2) + sizeof(petal3) + sizeof(petal4) + sizeof(petal5) + sizeof(petal6);

                        uint8_t* petalLEDMem = new uint8_t[ pixelCount ];

                        uint8_t* ptr = petalLEDMem;

                        memcpy(ptr, petal1, sizeof(petal1)); ptr += sizeof(petal1);
                        memcpy(ptr, petal2, sizeof(petal2)); ptr += sizeof(petal2);
                        memcpy(ptr, petal3, sizeof(petal3)); ptr += sizeof(petal3);
                        memcpy(ptr, petal4, sizeof(petal4)); ptr += sizeof(petal4);
                        memcpy(ptr, petal5, sizeof(petal5)); ptr += sizeof(petal5);
                        memcpy(ptr, petal6, sizeof(petal6)); ptr += sizeof(petal6);

                        return std::unique_ptr<uint8_t[]>(petalLEDMem);
                    }
                }
                break;

                case PETAL_NORMAL:
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

                    if( petalMovement_ == PETAL_MOVEMENT_ROTATE )
                    {
                        pixelCount = sizeof(petal1);
                        uint8_t* petalLEDMem = new uint8_t[pixelCount];
                        memcpy(petalLEDMem, petalArrays[petalIndex], pixelCount);

                        return std::unique_ptr<uint8_t[]>(petalLEDMem);
                    }
                    else
                    {
                        //all on!
                        pixelCount = sizeof(petal1) + sizeof(petal2) + sizeof(petal3) + sizeof(petal4) + sizeof(petal5) + sizeof(petal6);

                        uint8_t* petalLEDMem = new uint8_t[ pixelCount ];

                        uint8_t* ptr = petalLEDMem;

                        memcpy(ptr, petal1, sizeof(petal1)); ptr += sizeof(petal1);
                        memcpy(ptr, petal2, sizeof(petal2)); ptr += sizeof(petal2);
                        memcpy(ptr, petal3, sizeof(petal3)); ptr += sizeof(petal3);
                        memcpy(ptr, petal4, sizeof(petal4)); ptr += sizeof(petal4);
                        memcpy(ptr, petal5, sizeof(petal5)); ptr += sizeof(petal5);
                        memcpy(ptr, petal6, sizeof(petal6)); ptr += sizeof(petal6);

                        return std::unique_ptr<uint8_t[]>(petalLEDMem);
                    }
                }
                break;

                case PETAL_STEM:
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

                    if( petalMovement_ == PETAL_MOVEMENT_ROTATE )
                    {
                        uint8_t* petalLEDMem = new uint8_t[sizeof(petal1)];
                        memcpy(petalLEDMem, petalArrays[petalIndex], sizeof(petal1));

                        pixelCount = sizeof(petal1);
                        return std::unique_ptr<uint8_t[]>(petalLEDMem);
                    }
                    else
                    {
                        //all on!
                        pixelCount = sizeof(petal1) + sizeof(petal2) + sizeof(petal3) + sizeof(petal4) + sizeof(petal5) + sizeof(petal6);

                        uint8_t* petalLEDMem = new uint8_t[ pixelCount ];

                        uint8_t* ptr = petalLEDMem;

                        memcpy(ptr, petal1, sizeof(petal1)); ptr += sizeof(petal1);
                        memcpy(ptr, petal2, sizeof(petal2)); ptr += sizeof(petal2);
                        memcpy(ptr, petal3, sizeof(petal3)); ptr += sizeof(petal3);
                        memcpy(ptr, petal4, sizeof(petal4)); ptr += sizeof(petal4);
                        memcpy(ptr, petal5, sizeof(petal5)); ptr += sizeof(petal5);
                        memcpy(ptr, petal6, sizeof(petal6)); ptr += sizeof(petal6);

                        return std::unique_ptr<uint8_t[]>(petalLEDMem);
                    }
                }
                break;

                case PETAL_ROOTS:
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

                    if( petalMovement_ == PETAL_MOVEMENT_ROTATE )
                    {
                        uint8_t* petalLEDMem = new uint8_t[sizeof(petal1)];
                        memcpy(petalLEDMem, petalArrays[petalIndex], sizeof(petal1));

                        pixelCount = sizeof(petal1);
                        return std::unique_ptr<uint8_t[]>(petalLEDMem);
                    }
                    else
                    {
                        //all on!
                        pixelCount = sizeof(petal1) + sizeof(petal2) + sizeof(petal3) + sizeof(petal4) + sizeof(petal5) + sizeof(petal6);

                        uint8_t* petalLEDMem = new uint8_t[ pixelCount ];

                        uint8_t* ptr = petalLEDMem;

                        memcpy(ptr, petal1, sizeof(petal1)); ptr += sizeof(petal1);
                        memcpy(ptr, petal2, sizeof(petal2)); ptr += sizeof(petal2);
                        memcpy(ptr, petal3, sizeof(petal3)); ptr += sizeof(petal3);
                        memcpy(ptr, petal4, sizeof(petal4)); ptr += sizeof(petal4);
                        memcpy(ptr, petal5, sizeof(petal5)); ptr += sizeof(petal5);
                        memcpy(ptr, petal6, sizeof(petal6)); ptr += sizeof(petal6);

                        return std::unique_ptr<uint8_t[]>(petalLEDMem);
                    }
                }
                break;

                default:
                break;
            }

            return std::unique_ptr<uint8_t[]>(nullptr);
        };

    private:
        PETAL_TYPE_T petalType_;
        PETAL_MOVEMENT_T petalMovement_;
        uint32_t petalChangeTimeInMS_;
        uint8_t petalOrder_[6];
};
#else



class PetalPixelProvider : public LEDPixelProvider {
public:
    enum PETAL_TYPE_T {
        PETAL_JUST_TIP,
        PETAL_NORMAL,
        PETAL_STEM,
        PETAL_ROOTS
    };

    enum PETAL_MOVEMENT_T {
        PETAL_MOVEMENT_ALL_ON,
        PETAL_MOVEMENT_ROTATE
    };

    PetalPixelProvider(PETAL_TYPE_T petalType, PETAL_MOVEMENT_T movement, uint32_t petalChangeTimeInMS)
        : petalType_(petalType), petalMovement_(movement), petalChangeTimeInMS_(petalChangeTimeInMS),
          petalOrder_({1, 4, 0, 3, 5, 2}) {}

    std::unique_ptr<uint8_t[]> getPixels(const uint32_t timeInMS, uint8_t &pixelCount) override {
        const uint8_t petalIndex = petalOrder_[(timeInMS / petalChangeTimeInMS_) % petalOrder_.size()];
        return generatePetalPixels(petalIndex, pixelCount);
    }

private:
    PETAL_TYPE_T petalType_;
    PETAL_MOVEMENT_T petalMovement_;
    uint32_t petalChangeTimeInMS_;
    std::vector<uint8_t> petalOrder_;

    std::unique_ptr<uint8_t[]> generatePetalPixels(uint8_t petalIndex, uint8_t &pixelCount) {
        // Define petals based on PETAL_TYPE_T
        // This should be replaced with actual implementation
        const std::vector<std::vector<uint8_t>> petals = {
            /* PETAL_JUST_TIP */ { 3, 9, 15, 21, 27, 33 },
            /* PETAL_NORMAL */   { 1, 2, 3, 4, 7, 8, 9, 10, 13, 14, 15, 16, 19, 20, 21, 22, 25, 26, 27, 28, 31, 32, 33, 34 },
            /* PETAL_STEM */     { 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 18, 19, 20, 21, 22, 24, 25, 26, 27, 28, 30, 31, 32, 33, 34 },
            /* PETAL_ROOTS */    { 35, 0, 1, 2, 3, 4, 5, 5, 6, 7, 8, 9, 10, 11, 11, 12, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 22, 23, 23, 24, 25, 26, 27, 28, 29, 29, 30, 31, 32, 33, 34, 35 }
        };

        const std::vector<uint8_t>& currentPetal = petals[petalType_];

        if (petalMovement_ == PETAL_MOVEMENT_ROTATE) {
            // Handle rotate movement
            pixelCount = currentPetal.size();
            std::unique_ptr<uint8_t[]> petalLEDMem(new uint8_t[pixelCount]);
            std::copy(currentPetal.begin(), currentPetal.end(), petalLEDMem.get());
            return petalLEDMem;
        } else {
            // Handle all on movement
            // This part of the code assumes that all petals need to be combined
            pixelCount = std::accumulate(petals.begin(), petals.end(), size_t(0),
                                         [](size_t sum, const std::vector<uint8_t>& petal) { return sum + petal.size(); });

            std::unique_ptr<uint8_t[]> petalLEDMem(new uint8_t[pixelCount]);
            uint8_t* ptr = petalLEDMem.get();
            for (const auto& petal : petals) {
                std::copy(petal.begin(), petal.end(), ptr);
                ptr += petal.size();
            }
            return petalLEDMem;
        }
    }
};


#endif


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
        ChaseColorProvider( const uint32_t color, const uint8_t chaseSize, const uint8_t ledOffset, const bool clockwise )
          : color_(color), chaseSize_(chaseSize), ledOffset_(ledOffset), clockwise_(clockwise) {};

        std::unique_ptr<uint32_t[]> getColours( const uint32_t numberPixels, const uint32_t timeInMS );

    private:
        uint32_t color_;
        uint8_t chaseSize_;
        uint8_t ledOffset_;
        bool clockwise_;
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
            const uint8_t ledSequence[] = {28, 12, 21, 20, 19, 35, 23, 7, 10, 3, 17, 1, 18, 13, 6, 33, 15, 32, 0, 26, 11, 16, 27, 24, 34, 30, 9, 4, 14, 8, 5, 31, 25, 29, 22, 2};

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



/// Special Effect Colour Provider ///

class LEDSpecialEffectProvider {
    public:
        virtual std::unique_ptr<uint32_t[]> modifyColours( uint32_t *leds, const uint32_t ledCount, const uint32_t timeInMS );
};


class BlurSpecialEffectProvider : public LEDSpecialEffectProvider {
    public:
        BlurSpecialEffectProvider( void ) {

        };

        std::unique_ptr<uint32_t[]> modifyColours( uint32_t *leds, const uint32_t ledCount, const uint32_t timeInMS ) {

            //every time we get the colours, in, we look at the lastLEDs_ and we move the LED colour towards the target LED colour by 25% on a per R,G and B basis
            //we then return the new colour

            //we can modify in-place the leds and then copy over to the last leds and return a copy of them

            //iterate over all the input LED array and process
            for( uint8_t i = 0; i < ledCount; i++ ) {
                //get the input LED
                uint32_t inLED = leds[i];

                //get the last LED
                uint32_t lastLED = lastLEDs_[i];

                //get the R,G and B values for the input LED
                uint8_t inR = (inLED >> 16) & 0xFF;
                uint8_t inG = (inLED >>  8) & 0xFF;
                uint8_t inB = (inLED >>  0) & 0xFF;

                //get the R,G and B values for the last LED
                uint8_t lastR = (lastLED >> 16) & 0xFF;
                uint8_t lastG = (lastLED >>  8) & 0xFF;
                uint8_t lastB = (lastLED >>  0) & 0xFF;

                //calculate the new R,G and B values
                uint8_t newR = (inR + lastR) / 2;
                uint8_t newG = (inG + lastG) / 2;
                uint8_t newB = (inB + lastB) / 2;

                //set the new LED
                lastLEDs_[i] = LEDEffect::MakeColor(newR, newG, newB);
            }

            return std::unique_ptr<uint32_t[]>(lastLEDs_);
        }

    private:
        uint32_t lastLEDs_[36];
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


class LEDSpecialEffect : LEDEffect {
  public:
    LEDSpecialEffect( LEDSpecialEffectProvider &specialEffectProvider )
      : specialEffectProvider_(specialEffectProvider) {};

    void process( uint32_t *leds, const uint32_t ledCount, const uint32_t time );

  private:
    LEDSpecialEffectProvider &specialEffectProvider_;
};
