#include "Particle.h"

#include "RgbStrip.h"
#include "NtcThermistor.h"
#include "clickButton.h"
#include "Settings.h"
#include "AudioPlayer.h"
#include "MP3Player.h"
#include "TonePlayer.h"

#define FIXED_AUDIO_TONE
#define FIXED_MP3_PLAYBACK

#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO);

//The particle logo on the front is a button - this is the controller for it
static constexpr int TOUCH_PIN = D10;
ClickButton particleButton(TOUCH_PIN, LOW, CLICKBTN_PULLUP);

//The controller for the LEDs and the mode
RgbStrip *rgbStrip = NULL;
static RgbStrip::MODES_T mode = RgbStrip::MODE_SNOWFLAKE;

//our settings controller
Settings settings = Settings();

//audio interface
AudioPlayer audioPlayer = AudioPlayer();

//mp3 player
MP3Player mp3Player = MP3Player(&audioPlayer);

//tone player
TonePlayer tonePlayer = TonePlayer(&audioPlayer);

//list of songs to play and index of the current song
std::vector<String> songs;
uint32_t songIndex = 0;

void setup()
{
    // //wait for usb  to connect
    // waitFor(Serial.isConnected, 10000);

    // delay(10000);

    rgbStrip = new RgbStrip();

    //load our settings file
    settings.init();

    //get the led mode and set the mode variable
    String ledMode = settings.get("ledMode");
    if (ledMode.length() > 0) {
        mode = (RgbStrip::MODES_T)ledMode.toInt();
        rgbStrip->setMode(mode);
    }

    //configure the touch button
    pinMode(TOUCH_PIN, INPUT_PULLUP);

    // Setup button timers (all in milliseconds / ms)
    // (These are default if not set, but changeable for convenience)
    particleButton.debounceTime   = 20;   // Debounce timer in ms
    particleButton.multiclickTime = 250;  // Time limit for multi clicks
    particleButton.longClickTime  = 1000; // time until "held-down clicks" register

    // find the file in the assets
    auto assets = System.assetsAvailable();

    for (auto& asset: assets)
    {
        if (asset.name().endsWith(".mp3"))
        {
            songs.push_back(asset.name());
        }
    }

    //hardware watchdog
    Watchdog.init(WatchdogConfiguration().timeout(10s));
    Watchdog.start();

  #ifdef FIXED_AUDIO_TONE
      //play a two-tone beep boop when booting up
      tonePlayer.toneSequence( TonePlayer::TONE_SEQUENCE_BOOT );
  #endif
}


void loop()
{
    // Update button state
    particleButton.Update();

    //kick the watchdog
    Watchdog.refresh(); 

    //switch on particleButton.clicks
    switch( particleButton.clicks ) 
    {
        case 1:
            Serial.println("SINGLE click");
            //inc mode
            mode = (RgbStrip::MODES_T)((mode + 1) % RgbStrip::MODE_MAX);
            if( mode == RgbStrip::MODE_OFF ) { // don't allow off to be selected by default
                mode == RgbStrip::MODE_SNOWFLAKE;
            }
            rgbStrip->setMode(mode);

            //store the updated setting
            settings.set("ledMode", String(mode));
            settings.store();

            #ifdef FIXED_AUDIO_TONE
                //play a two-tone beep boop when switching the display mode
                //this will fail if already playing a song
                tonePlayer.toneSequence( TonePlayer::TONE_SEQUENCE_TWO_TONE );
            #endif
        break;

        case 2:
            Serial.println("DOUBLE click");
        break;

        case 3:
            Serial.println("TRIPLE click");
        break;

        case -1:
            Serial.println("SINGLE LONG click");

            #ifdef FIXED_MP3_PLAYBACK
                //play the next song in the list
                //it allows max of 1 item to be queued
                mp3Player.play(songs[songIndex]);
                songIndex = (songIndex + 1) % songs.size();
            #endif
        break;
    }
}


//USB controls
#if 0

    JSONValue getValue(const JSONValue& obj, const char* name) {
        JSONObjectIterator it(obj);
        while (it.next()) {

          // // Debug
          String name(it.name());
          String value(it.value().toString());
          Log.info("%s %s", name.c_str(), value.c_str());
          // Debug

            if (it.name() == name) {
                return it.value();
            }
        }
        return JSONValue();
    }

    uint8_t brightness = 0;

    uint32_t pixels[36] = {0};

    uint8_t glowSequenceLED = 0;
    uint8_t glowSequenceDelay = 0;

    void ctrl_request_custom_handler(ctrl_request* req) {

        // uint16_t size; // Size of this structure
        // uint16_t type; // Request type
        // char* request_data; // Request data
        // size_t request_size; // Size of the request data
        // char* repvvly_data; // Reply data
        // size_t reply_size; // Size of the reply data
        // void* channel; // Request channel (used internally)

        LOG(INFO, "request_data: %.*s", req->request_size, req->request_data );

        auto d = JSONValue::parse(req->request_data, req->request_size);
        SPARK_ASSERT(d.isObject());
        JSONValue data_ = std::move(d);

        JSONObjectIterator iter(data_);
        while(iter.next()) {
            Log.info("key=%s value=%s", 
              (const char *) iter.name(), 
              (const char *) iter.value().toString());

            if (iter.name() == "setBrightness") {
                brightness = iter.value().toInt();

                LOG(INFO, "brightness: %d", brightness);
            }
            else if (iter.name() == "setLeds") {
                //print out jstr as a cstr
                const char * const s = iter.value().toString().data();

                String leds(s);

                //leds is a string that is 36 sets of RGB HEX strings that are 6 ASCII tightly packed

                //Parse these into a 1D array of UINT32s
                //Then pass that array to the RgbStrip class to display the colors

                for (int i = 0; i < 36; i++) {
                    String hex = leds.substring(i * 6, i * 6 + 6);
                    uint32_t color = strtol(hex.c_str(), NULL, 16);
                    pixels[i] = color;
                }

                //print out the first pixel as RGB
                uint32_t color = pixels[0];
                uint8_t r = (color >> 16) & 0xFF;
                uint8_t g = (color >> 8) & 0xFF;
                uint8_t b = color & 0xFF;

                LOG(INFO, "r=%d g=%d b=%d", r, g, b);

                rgbStrip->set_pixels(pixels, brightness, 36);
            }
            else if (iter.name() == "setLed") {
                //print out jstr as a cstr
                const char * const s = iter.value().toString().data();

                String led(s);

                //split the string by :
                int pos = led.indexOf(':');

                String index = led.substring(0, pos);
                String hex = led.substring(pos + 1);

                uint32_t indexInt = strtol(index.c_str(), NULL, 16);
                uint32_t color = strtol(hex.c_str(), NULL, 16);

                //log this
                LOG(INFO, "index=%s hex=%s", index.c_str(), hex.c_str());
                
                pixels[indexInt] = color;

                rgbStrip->set_pixels(pixels, brightness, 36);
            }
            //support this command
            // {setGlowSequenceLED: 0, setGlowSequenceDelay: 5, setGlowSequence: ledstring}
            // in this the ledstring is N pixels long and each pixel is 6 hex chars
            // the led is the LED to glow (0 to 35)
            // the delay is the time between pixels in ms
            else if (iter.name() == "setGlowSequenceLED") {
                glowSequenceLED = iter.value().toInt();
            }
            else if (iter.name() == "setGlowSequenceDelay") {
                glowSequenceDelay = iter.value().toInt();
            }
            else if (iter.name() == "setGlowSequence") {
                //print out jstr as a cstr
                const char * const s = iter.value().toString().data();

                String glowString(s);

                //calculate the length of the sequence - its the string length / 6 (6 chars per pixel)
                const uint32_t sequenceLength = glowString.length() / 6;

                uint32_t *sequence = (uint32_t *)malloc(sequenceLength * sizeof(uint32_t));

                for (int i = 0; i < sequenceLength; i++) {
                    String hex = glowString.substring(i * 6, i * 6 + 6);
                    uint32_t color = strtol(hex.c_str(), NULL, 16);
                    sequence[i] = color;
                }

                //set each pixel in the sequence with teh delay between them using the led number
                for (int i = 0; i < sequenceLength; i++) {
                    pixels[glowSequenceLED] = sequence[i];
                    rgbStrip->set_pixels(pixels, brightness, 36);
                    delay(glowSequenceDelay);
                }

                free( sequence );
            }
        }

        data_ = JSONValue();

        // auto d = JSONValue::parse(req->request_data, req->request_size);
        // SPARK_ASSERT(d.isObject());
        // JSONValue data_ = std::move(d);

        // //is the key "setBrightness" in the JSON object?

        // JSONString jstr = getValue(data_, "cmd").toString();

        // //if the cmd is "setBrightness" then get the brightness value
        // if (jstr == "setBrightness") {

        //     JSONObjectIterator iter(d);
        //     while (iter.next()) {
        //         if (iter.name() == "brightness") {
        //             brightness = iter.value().toInt();
        //         }
        //     }
        // }
        // //else if the string is setLeds then get the leds value
        // else if (jstr == "setLeds") {
        //     jstr = getValue(data_, "leds").toString();

        //     //print out jstr as a cstr
        //     const char * const s = jstr.data();

        //     String leds(s);

        //     //leds is a string that is 36 sets of RGB HEX strings that are 6 ASCII tightly packed

        //     //Parse these into a 1D array of UINT32s
        //     //Then pass that array to the RgbStrip class to display the colors

        //     uint32_t pixels[36];

        //     for (int i = 0; i < 36; i++) {
        //         String hex = leds.substring(i * 6, i * 6 + 6);
        //         uint32_t color = strtol(hex.c_str(), NULL, 16);
        //         pixels[i] = color;
        //     }

        //     rgbStrip->set_pixels(pixels, brightness, 36);
        // }

        const bool ok = true;

        system_ctrl_set_result(req, ok ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_NOT_SUPPORTED, nullptr, nullptr, nullptr);
    }


#endif