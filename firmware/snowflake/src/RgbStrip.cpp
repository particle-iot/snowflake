#include "RgbStrip.h"
#include "LEDEffect.h"

#define PIXEL_PIN SPI
#define PIXEL_COUNT 36
#define PIXEL_TYPE WS2812B

#define MAX_FRAMERATE 30

//colors
const uint32_t hanukka_blue = LEDEffect::MakeColor(0, 0, 32);
const uint32_t hanukka_blue_highlight = LEDEffect::MakeColor(3, 3, 105);

const uint32_t snowflake_white = LEDEffect::MakeColor(169, 169, 204);
const uint32_t snowflake_white_highlight = LEDEffect::MakeColor(255, 255, 255);

const uint32_t chase_red = LEDEffect::MakeColor(245, 58, 12);
const uint32_t chase_red_highlight = LEDEffect::MakeColor(255, 0, 0);

const uint32_t green_base = LEDEffect::MakeColor(0, 92, 0);
const uint32_t green_highlight = LEDEffect::MakeColor(0, 196, 0);


RgbStrip::RgbStrip() 
: mode_(MODES_T::MODE_OFF) {
    strip_ = new Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

    thread_ = new Thread("rgbThread", [this]()->os_thread_return_t{

        //Shared pixel providers
        InnerCirclePixelProvider innerCirclePixelProvider = InnerCirclePixelProvider();
        AllPixelsProvider allPixelsProvider = AllPixelsProvider();
        EveryNPixelProvider everyNPixelProvider = EveryNPixelProvider( 5 );
        PetalPixelProvider petalPixelProvider = PetalPixelProvider( true, false, 2500 );

        //Shared colour providers
        RainbowColorProvider rainbowColorProvider = RainbowColorProvider();
        SparkleColorProvider sparkleColorProvider = SparkleColorProvider( LEDEffect::MakeColor(255, 255, 255), 14, 1800, 10000 );

        //MODE - MODE_SNOWFLAKE
            FixedColorProvider fixedColorProviderWhite = FixedColorProvider( snowflake_white );
            GlowColorProvider glowColorProviderWhite = GlowColorProvider( snowflake_white, snowflake_white_highlight, 2500 );

            LEDEffectPixelAndColor snowFlakeEffects[] = {
                LEDEffectPixelAndColor( allPixelsProvider, fixedColorProviderWhite ),
                LEDEffectPixelAndColor( allPixelsProvider, sparkleColorProvider ),
                //LEDEffectPixelAndColor( petalPixelProvider, glowColorProviderWhite ), //THIS IS CURRENLY GITCHING
            };

        //MODE - MODE_HANUKKA
            FixedColorProvider fixedColorProviderBlue = FixedColorProvider( hanukka_blue );
            GlowColorProvider glowColorProviderBlue = GlowColorProvider( hanukka_blue, hanukka_blue_highlight, 2500 );

            LEDEffectPixelAndColor hanukkaEffects[] = {
                LEDEffectPixelAndColor( allPixelsProvider, fixedColorProviderBlue ),
                LEDEffectPixelAndColor( allPixelsProvider, sparkleColorProvider ),
                LEDEffectPixelAndColor( petalPixelProvider, glowColorProviderBlue ),
            };

        //MODE - MODE_RAINBOW
            LEDEffectPixelAndColor rainbowEffects[] = {
                LEDEffectPixelAndColor( allPixelsProvider, rainbowColorProvider ),
                LEDEffectPixelAndColor( allPixelsProvider, sparkleColorProvider )
            };

        //MODE - MODE_CHASE_RED
            FixedColorProvider fixedColorProviderChase = FixedColorProvider( green_base );
            GlowColorProvider glowColorProviderChase = GlowColorProvider( green_base, green_highlight, 2500 );
            ChaseColorProvider chaseColorProvider = ChaseColorProvider( snowflake_white_highlight, 3 );

            LEDEffectPixelAndColor chaseRedEffects[] = {
                LEDEffectPixelAndColor( allPixelsProvider, fixedColorProviderChase ),
                LEDEffectPixelAndColor( petalPixelProvider, glowColorProviderChase ),
                LEDEffectPixelAndColor( innerCirclePixelProvider, chaseColorProvider ),
            };

        //calculate the framerate
        uint32_t frameCount = 0;
        uint32_t lastFrameTime = millis();

        while( true )
        {
            strip_->begin();
            strip_->setBrightness(25);

            uint32_t leds[36] = {0};

            uint32_t timeNow = millis();

            //process whichever mode is activated
            switch( mode_ )
            {
                case MODE_SNOWFLAKE:
                    //process all of the effects in snowFlakeEffects 1 at a time
                    for (size_t i = 0; i < (sizeof(snowFlakeEffects) / sizeof(LEDEffectPixelAndColor)); i++)
                    {
                        snowFlakeEffects[i].process( leds, 36, timeNow );
                    }
                break;

                case MODE_HANUKKA:
                    //process all of the effects in snowFlakeEffects 1 at a time
                    for (size_t i = 0; i < (sizeof(hanukkaEffects) / sizeof(LEDEffectPixelAndColor)); i++)
                    {
                        hanukkaEffects[i].process( leds, 36, timeNow );
                    }
                break;

                case MODE_RAINBOW:
                    //process all of the effects in snowFlakeEffects 1 at a time
                    for (size_t i = 0; i < (sizeof(rainbowEffects) / sizeof(LEDEffectPixelAndColor)); i++)
                    {
                        rainbowEffects[i].process( leds, 36, timeNow );
                    }
                break;

                case MODE_CHASE_RED:
                    //process all of the effects in snowFlakeEffects 1 at a time
                    for (size_t i = 0; i < (sizeof(chaseRedEffects) / sizeof(LEDEffectPixelAndColor)); i++)
                    {
                        chaseRedEffects[i].process( leds, 36, timeNow );
                    }
                break;

                default:
                    break;
            }

            //copy the results to the strip
            for (int i = 0; i < strip_->numPixels(); i++) {
                strip_->setPixelColor(i, leds[i]);
            }

            strip_->show();

            frameCount++;

            //if the last frametime is over 10 seconds, calculate the FPS since
            if( (timeNow - lastFrameTime) > 10000 )
            {
                LOG(INFO, "FPS: %d", frameCount / 10);

                frameCount = 0;
                lastFrameTime = timeNow;
            }
  
            //delay enough to round timeNow up to the next frame
            int32_t delayTime = (1000 / MAX_FRAMERATE) - (millis() - timeNow);
            if( delayTime > 0 )
            {
                delay( delayTime );
            }
        }
    });
}

RgbStrip::~RgbStrip() {
    delete strip_;
    delete thread_;
}
