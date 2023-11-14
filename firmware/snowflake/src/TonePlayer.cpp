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

void TonePlayer::tone( const uint32_t freq, const uint32_t duractionInMS ) {
    //what are we playing?
    Log.info("Playing tone: %ldHz for %ldms", freq, duractionInMS);

    if( 0 == audioPlayer->aquireLock() )
    {
        audioPlayer->setOutput(HAL_AUDIO_MODE_MONO, HAL_AUDIO_SAMPLE_RATE_16K, HAL_AUDIO_WORD_LEN_16);

        //generate a constant wave form that is split into buckets of 1024 samples based on the given freq
        //the generated waveform should be a sine wave below max volume
      
        const uint16_t samplesPerBucket = 1024;
        const uint16_t sampleRate = 16000;
        const uint16_t maxVolume = (32767 / 100) * 80; //80% of max volume

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
            for (uint16_t i = 0; i < bucketSize; i++) {
                //calculate the sample value
                const float sampleValue = sin(2 * M_PI * freq * (samplesWritten + i) / sampleRate);

                //convert the sample value to a 16bit signed int
                bucket[i] = (int16_t)(sampleValue * maxVolume);
            }

            //write the bucket to the audio output
            audioPlayer->playBuffer((const uint16_t*)bucket, bucketSize * sizeof(int16_t));

            //increment the samples written
            samplesWritten += samplesPerBucket;
        }

        //free the memory
        free(bucket);

        //terminate the audio output
        audioPlayer->releaseLock();
    }
    else
    {
        Log.info("Failed to aquire audio lock - skipping tone");
    }
}


void TonePlayer::toneSequence( const TONE_SEQUENCE_T sequence  )
{
    //Playing a tone?
    Log.info("Playing tone sequence: %d", sequence);

    //tone player
    const TONE_T *tones = (sequence == TONE_SEQUENCE_TWO_TONE ? two_tone_tones : NULL);
    const uint32_t numTones = (sequence == TONE_SEQUENCE_TWO_TONE ? sizeof(two_tone_tones) / sizeof(TONE_T) : 0);

    if( tones != NULL )
    {
        //if a tone frequency_Hz is 0, its a delay otherwise play the tone
        for (uint32_t i = 0; i < numTones; i++) {
            if (tones[i].frequency_Hz == 0) {
                delay(tones[i].time_ms);
            }
            else {
                tone(tones[i].frequency_Hz, tones[i].time_ms);
            }
        }
    }
}
