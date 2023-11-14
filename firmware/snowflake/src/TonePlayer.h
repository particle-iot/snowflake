#pragma once

#include "Particle.h"
#include "AudioPlayer.h"

class TonePlayer
{
  public:
      TonePlayer( AudioPlayer* audioPlayer ) : audioPlayer(audioPlayer) {}

      void tone( const uint32_t freq, const uint32_t duractionInMS );

      typedef enum {
        TONE_SEQUENCE_TWO_TONE,
        TONE_SEQUENCE_BOOT,

        TONE_SEQUENCE_MAX
        
      } TONE_SEQUENCE_T;

      void toneSequence( const TONE_SEQUENCE_T sequence );

  private:

    //reference to the global audio output
    AudioPlayer* audioPlayer;
};
