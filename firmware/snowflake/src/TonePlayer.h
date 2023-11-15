#pragma once

#include "Particle.h"
#include "AudioPlayer.h"

class TonePlayer
{
  public:
      TonePlayer( AudioPlayer* audioPlayer );

      typedef enum {
        TONE_SEQUENCE_TWO_TONE,
        TONE_SEQUENCE_BOOT,

        TONE_SEQUENCE_MAX
        
      } TONE_SEQUENCE_T;

      void play( const TONE_SEQUENCE_T sequence ) {
          const int success = os_queue_put(queue_, &sequence, CONCURRENT_WAIT_FOREVER, 0);
          if( 0 != success ) {
              Log.error("TonePlayer::play(%d) failed to put tone in queue", sequence);
          }
      }

  private:

    void doTone( const uint32_t freq, const uint32_t duractionInMS );

    void playToneSequence( const TONE_SEQUENCE_T sequence );

    //reference to the global audio output
    AudioPlayer* audioPlayer_;
    os_queue_t queue_;
    Thread* thread_;
};
