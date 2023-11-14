#include "LEDEffect.h"
#include "neopixel.h"

//////

std::unique_ptr<uint32_t[]> ChaseColorProvider::getColours( const uint32_t numberPixels, const uint32_t timeInMS ) {
    std::unique_ptr<uint32_t[]> colours( new uint32_t[numberPixels] );

    const uint32_t timerBreak = 140;

    //fill in all the pixels to transparent
    for (uint16_t i = 0; i < numberPixels; i++ )
    {
        colours[i] = LEDEffect::TRANSPARENT_COLOR;
    }

    //if we are clock wise...
    if( clockwise_ )
    {
        //create a chase around the list of pixels. The chase is n leds long
        const uint8_t chaseStart = (((timeInMS / timerBreak) + ledOffset_) % numberPixels);

        //we want to write chaseSize_ number of pixels in a trail
        //the last pixel is at full brightness, the first is at 10%
        for (size_t i = 0; i < chaseSize_; i++)
        {
            colours[(chaseStart + i) % numberPixels] = LEDEffect::ScaleColor(color_, 10 + (i * 65 / chaseSize_));
        }
    }
    else
    {
        //create a chase around the list of pixels. The chase is n leds long
        //chase start goes backwards from the start when in anti-clockwise mode
        const uint8_t chaseStart = numberPixels - (((timeInMS / timerBreak) + ledOffset_) % numberPixels);
    
        //the chase goes backwards from the start when in anti-clockwise mode
        // - chaseStart is the last pixel... so we need to go backwards
        // - the last pixel is at full brightness, the first is at 10%
        for (size_t i = 0; i < chaseSize_; i++)
        {
            //calculate the pixel we are writing to
            int8_t pixel = chaseStart - i;

            //if the pixel is negative, then we need to wrap around
            if( pixel < 0 ) {
                pixel = numberPixels + pixel;
            }

            //set the colour
            colours[pixel] = LEDEffect::ScaleColor(color_, 10 + (i * 65 / chaseSize_));
        }
    }

    return colours;
};


std::unique_ptr<uint32_t[]> RainbowColorProvider::getColours( const uint32_t numberPixels, const uint32_t timeInMS ) {
    std::unique_ptr<uint32_t[]> colours( new uint32_t[numberPixels] );

    //our rainbow state 'j' changes every 50ms
    uint32_t j = (timeInMS / 50) & 255;

    for (uint16_t i = 0; i < numberPixels; i++) {
        colours[i] = LEDEffect::colourWheel((i + j) & 255);
    }

    return colours;
};


std::unique_ptr<uint32_t[]> FixedColorProvider::getColours( const uint32_t numberPixels, const uint32_t timeInMS ) {
    std::unique_ptr<uint32_t[]> colours( new uint32_t[numberPixels] );

    //fill with our single color
    for (uint16_t i = 0; i < numberPixels; i++ )
    {
        colours[i] = color_;
    }

    return colours;
};


std::unique_ptr<uint32_t[]> GlowColorProvider::getColours( const uint32_t numberPixels, const uint32_t timeInMS ) {
    std::unique_ptr<uint32_t[]> colours( new uint32_t[numberPixels] );

    //we want to glow the LEDs between base_color_ and glow_color_ between glowTimeWindow_
    //a glow sequence has a lead in of glowTimeWindow_/2 and an lead out of glowTimeWindow_/2

    //calculate the time since the last glow
    uint32_t sequence = (timeInMS % (glowTimeWindow_));

    //print out if we are in the pre-roll, main sequence 1st half, main seqnece second half or post roll
    //LOG(INFO, "sequence: %d %d %d", sequence, glowTimeWindow_, glowTimeWindow_/2);

    //log the to and from colours
    // LOG(INFO, "GlowColorProvider from: %d %d %d to %d %d %d",
    //       (base_color_ >> 16) & 0xFF, (base_color_ >> 8) & 0xFF, (base_color_ & 0xFF),
    //       (glow_color_ >> 16) & 0xFF, (glow_color_ >> 8) & 0xFF, (glow_color_ & 0xFF));

    //if its in the pre or post roll, its just base colour
    if( ( sequence < (glowTimeWindow_/4) ) || ( sequence > ((glowTimeWindow_/2)+(glowTimeWindow_/4)) ) )
    {
      //fill base colour
      for (uint16_t i = 0; i < numberPixels; i++ )
      {
          colours[i] = base_color_;
      }
    }
    else
    {
        //calculute the sequence into the glow
        sequence -= (glowTimeWindow_/4);

        //we are in the first half of the glow
        //we need to scale the colour between base_color_ and glow_color_ based on the sequence
        uint32_t r = (base_color_ >> 16) & 0xFF;
        uint32_t g = (base_color_ >> 8) & 0xFF;
        uint32_t b = base_color_ & 0xFF; 

        const uint32_t red_delta = ((glow_color_ >> 16) & 0xFF) - r;
        const uint32_t green_delta = ((glow_color_ >> 8) & 0xFF) - g;
        const uint32_t blue_delta = ((glow_color_) & 0xFF) - b;

        uint32_t red_delta_scaled = 0;
        uint32_t green_delta_scaled = 0;
        uint32_t blue_delta_scaled = 0;

        uint32_t red_delta_scaled_by_seq = 0;
        uint32_t green_delta_scaled_by_seq = 0;
        uint32_t blue_delta_scaled_by_seq = 0;

        //calculate the r, g, b values
        if( sequence < (glowTimeWindow_/4) )
        {
            //calculate the delta between the base and glow colours
            red_delta_scaled = red_delta * sequence;
            green_delta_scaled = green_delta * sequence;
            blue_delta_scaled = blue_delta * sequence;

            red_delta_scaled_by_seq = red_delta_scaled / (glowTimeWindow_/4);
            green_delta_scaled_by_seq = green_delta_scaled / (glowTimeWindow_/4);
            blue_delta_scaled_by_seq = blue_delta_scaled / (glowTimeWindow_/4);
        }
        else
        {
            //calculate the delta between the base and glow colours
            red_delta_scaled = red_delta * ((glowTimeWindow_/2) - sequence);
            green_delta_scaled = green_delta * ((glowTimeWindow_/2) - sequence);
            blue_delta_scaled = blue_delta * ((glowTimeWindow_/2) - sequence);


            red_delta_scaled_by_seq = red_delta_scaled / (glowTimeWindow_/4);
            green_delta_scaled_by_seq = green_delta_scaled / (glowTimeWindow_/4);
            blue_delta_scaled_by_seq = blue_delta_scaled / (glowTimeWindow_/4);            
        }

        //log all the values
        //LOG(INFO, "r: %d %d %d %d", r, red_delta, red_delta_scaled, red_delta_scaled_by_seq );
        //LOG(INFO, "g: %d %d %d %d", g, green_delta, green_delta_scaled, green_delta_scaled_by_seq );
        //LOG(INFO, "b: %d %d %d %d", b, blue_delta, blue_delta_scaled, blue_delta_scaled_by_seq );

        //scale the colour
        r += red_delta_scaled_by_seq;
        g += green_delta_scaled_by_seq;
        b += blue_delta_scaled_by_seq;

        //fill the colours
        for (uint16_t i = 0; i < numberPixels; i++ )
        {
            colours[i] = LEDEffect::MakeColor(r, g, b);
        }
    }

    return colours;
};



std::unique_ptr<uint32_t[]> SparkleColorProvider::getColours( const uint32_t numberPixels, const uint32_t timeInMS ) {
    std::unique_ptr<uint32_t[]> colours( new uint32_t[numberPixels] );

    //for each sparkleLEDs_, lets calculate the colour based on the time
    for( uint8_t i = 0; i < sparkleLEDs_.size(); i++ ) {
        //get the sparkleLED
        SparkleLED sparkleLED = sparkleLEDs_[i];

        //has this started yet?!
        if( sparkleLED.startTime_ > timeInMS ) {
            //not ready to be processed yet!
        }
        else {
            //calculate the time since the sparkleLED started
            uint32_t timeSinceStart = timeInMS - sparkleLED.startTime_;

            //if the time since start is > 10 seconds, then we need to reset the sparkleLED
            if( timeSinceStart > sparkleTime_ ) {
                //reset the sparkleLED
                sparkleLED.startTime_ = timeInMS + random(0, sparkleSpreadTime_);
                sparkleLED.led_ = flashSequence_[flashSequenceIndex_];
                incFlashSequenceIndex();

                //update the vector
                sparkleLEDs_[i] = sparkleLED;
            }
            else {
                //calculate the colour based on the time since start
                uint8_t brightness = 0;

                if( timeSinceStart < (sparkleTime_/2)) {
                    //we are in the first (sparkleTime_/2) seconds, so we are increasing the brightness
                    //we need to scale the brightness between 0 and (sparkleTime_/2)
                    brightness = (timeSinceStart * 100) / (sparkleTime_/2);
                }
                else {
                    //we are in the last (sparkleTime_/2) seconds, so we are decreasing the brightness
                    //we need to scale the brightness between 100 and 0
                    brightness = 100 - (((timeSinceStart - (sparkleTime_/2)) * 100) / (sparkleTime_/2));
                }

                //set the colour
                colours[sparkleLED.led_] = LEDEffect::ScaleColor(color_, brightness);
            }
        }
    }

    return colours;
};



void LEDEffectPixelAndColor::process( uint32_t *leds, const uint32_t ledCount, const uint32_t timeInMS )
{
    //get the pixels from the pixel provider
    uint8_t pixelCount = 0;
    std::unique_ptr<uint8_t[]> pixels = pixelProvider_.getPixels( timeInMS, pixelCount );

    //get the colours from the colour provider based on the number of pixels
    std::unique_ptr<uint32_t[]> colours = colorProvider_.getColours( pixelCount, timeInMS );

    //apply the colours to the pixels
    for (uint16_t i = 0; i < pixelCount; i++ )
    {
        //if its not transparent, then set the colour
        if( colours[i] != LEDEffect::TRANSPARENT_COLOR ) {
            leds[pixels[i]] = colours[i];
        }
    }
};
