#include "AudioPlayer.h"
#include "TonePlayer.h"
#include <math.h>

#define M_PI  3.14159265358979323846

typedef struct {
    uint16_t frequency_Hz;
    uint16_t time_ms;
} TONE_T;

static const TONE_T two_tone_tones[] = {
    { 2000,     100 },
    { 0,        50 },
    { 4000,     100 },
    { 0,        50 }
};

static const TONE_T boot_up_tones[] = {
    { 2218,     38 },
    { 0,        77 },
    { 1319,     38 },
    { 0,        77 },
    { 2218,     38 },
    { 0,        77 },
    { 2637,     38 }
};

static const TONE_T alert1[] = {
    { 3520,     65 },
    { 0,        50 },
    { 1980,     65 },
    { 0,        50 },
    { 3520,     65 },
    { 0,        50 },
    { 1980,     65 },
    { 0,        50 },
    { 3520,     65 }
};

static const TONE_T alert2[] = {
    { 3951,     65 },
    { 0,        50 },
    { 2218,     65 },
    { 0,        50 },
    { 3951,     65 },
    { 0,        50 },
    { 2218,     65 },
    { 0,        50 },
    { 3951,     65 }
};

static const TONE_T alert3[] = {
    { 4186,     65 },
    { 0,        10 },
    { 2794,     65 },
    { 0,        10 },
    { 2218,     65 },
    { 0,        10 },
    { 1760,     65 }
};

static const TONE_T alert4[] = {
    { 1760,     65 },
    { 0,        10 },
    { 2218,     65 },
    { 0,        10 },
    { 2794,     65 },
    { 0,        10 },
    { 4186,     65 }
};

static const TONE_T sad_tone[] = {
    { 3520,     38 },
    { 0,        24 },
    { 3729,     38 },
    { 0,        24 },
    { 3322,     38 },
    { 0,        77 },
    { 2217,     38 },
    { 0,        24 },
    { 2349,     38 },
    { 0,        24 },
    { 2093,     38 },
    { 0,        77 },
    { 1760,     38 },
    { 0,        24 },
    { 1865,     38 },
    { 0,        24 },
    { 1397,     38 }
};


TonePlayer::TonePlayer( AudioPlayer* audioPlayer )
  : audioPlayer_(audioPlayer)
{
    os_queue_create(&queue_, sizeof(TONE_SEQUENCE_T), 1, NULL);

    thread_ = new Thread("toneplayer", [this]()->os_thread_return_t{

        while (1) {
            TONE_SEQUENCE_T toneSequence;

            if( 0 == os_queue_take(queue_, &toneSequence, CONCURRENT_WAIT_FOREVER, 0) ) {
               Log.info("TonePlayer:: got tone from queue: %d", toneSequence);

                playToneSequence(toneSequence);
            }
        }
    }, OS_THREAD_PRIORITY_NETWORK_HIGH, OS_THREAD_STACK_SIZE_DEFAULT_NETWORK );   
}


void TonePlayer::playToneSequence( const TONE_SEQUENCE_T sequence  )
{
    //Playing a tone?
    Log.info("Playing tone sequence: %d", sequence);

    if( 0 == audioPlayer_->aquireLock() )
    {
        audioPlayer_->setOutput(HAL_AUDIO_MODE_MONO, HAL_AUDIO_SAMPLE_RATE_16K, HAL_AUDIO_WORD_LEN_16);

        //tone player
        TONE_T *tones = NULL;
        uint32_t numTones = 0;
 
        switch( sequence )
        {
            case TONE_SEQUENCE_TWO_TONE:
                tones = (TONE_T*)two_tone_tones;
                numTones = sizeof(two_tone_tones) / sizeof(TONE_T);
            break;

            case TONE_SEQUENCE_BOOT:
                tones = (TONE_T*)boot_up_tones;
                numTones = sizeof(boot_up_tones) / sizeof(TONE_T);
            break;

            default:
                Log.info("Unknown tone sequence: %d", sequence);
            break;
        }

        if( tones != NULL )
        {
            //if a tone frequency_Hz is 0, its a delay otherwise play the tone
            for (uint32_t i = 0; i < numTones; i++)
            {
                if (tones[i].frequency_Hz == 0)
                {
                    delay(tones[i].time_ms);
                }
                else
                {
                    doTone(tones[i].frequency_Hz, tones[i].time_ms);
                }
            }
        }

        //terminate the audio output
        audioPlayer_->releaseLock();
    }
    else
    {
        Log.info("Failed to aquire audio lock - skipping tone");
    }
}

void TonePlayer::doTone( const uint32_t freq, const uint32_t duractionInMS ) {
    //what are we playing?
    //Log.info("Playing tone: %ldHz for %ldms", freq, duractionInMS);

    //generate a constant wave form that is split into buckets of 1024 samples based on the given freq
    //the generated waveform should be a sine wave below max volume
  
    const uint16_t samplesPerBucket = 512;
    const uint16_t sampleRate = 16000;
    const uint16_t maxVolume = (32767 / 100) * 70; //70% of max volume

    //calculate how many samples in total are needed for the duraction in ms we want to play for
    const uint32_t totalSamples = (sampleRate * duractionInMS) / 1000;

    //allocate a bucket for the samples
    int16_t* bucket = (int16_t*)malloc(samplesPerBucket * sizeof(int16_t));

    //loop over the total samples, filling in the bucket as we go. when its full or we have no samples left, write the audio output
    uint32_t samplesWritten = 0;

    while (samplesWritten < totalSamples) {

        //limit the max size of the bucket to fill. either the bucket or the remainined total samples
        const uint16_t bucketSize = min(samplesPerBucket, totalSamples - samplesWritten);

        //fill the bucket
        for (uint16_t i = 0; i < bucketSize; i++)
        {
            //calculate the sample value
            const float sampleValue = sin(2 * M_PI * freq * (samplesWritten + i) / sampleRate);

            //if we are in the last 24 samples, fade them to zero
            //if we are in the first 24 samples, fade them from zero
            //else play at max volume
            if (i < 24)
            {
                //fade from zero
                bucket[i] = (int16_t)(sampleValue * maxVolume * (i / 24.0f));
            }
            else if (i > (bucketSize - 24))
            {
                //fade to zero
                bucket[i] = (int16_t)(sampleValue * maxVolume * ((bucketSize - i) / 24.0f));
            }
            else
            {
                //convert the sample value to a 16bit signed int
                bucket[i] = (int16_t)(sampleValue * maxVolume);
            }
        }

        //write the bucket to the audio output
        audioPlayer_->playBuffer((const uint16_t*)bucket, bucketSize * sizeof(int16_t));

        //increment the samples written
        samplesWritten += samplesPerBucket;
    }

    //free the memory
    free(bucket);
}