#pragma once

#include "Particle.h"
#include "audio_lib/audio_hal.h"

class AudioPlayer
{
  static constexpr int SPEAKER_EN_PIN = D5;

  public:
      AudioPlayer() {
          os_mutex_create(&mutex_);
      }

      int aquireLock( void ) {
          const int ret = os_mutex_trylock(mutex_);

          if( ret == 0)
          {
              pinMode(SPEAKER_EN_PIN, OUTPUT);
              digitalWrite(SPEAKER_EN_PIN, 1);
          }
          
          return ret;
      }

      void releaseLock( void ) {
          digitalWrite(SPEAKER_EN_PIN, 0);
          pinMode(SPEAKER_EN_PIN, PIN_MODE_NONE);
          os_mutex_unlock(mutex_);
      }

      void setOutput( hal_audio_mode_t mode, hal_audio_sample_rate_t sampleRate, hal_audio_word_len_t wordLen ) {
          hal_audio_init(HAL_AUDIO_OUT_DEVICE_LINEOUT, mode, sampleRate, wordLen);

          pinMode(SPEAKER_EN_PIN, OUTPUT);
          digitalWrite(SPEAKER_EN_PIN, 1);
      }
  
      void playBuffer( const uint16_t *buffer, size_t size ) {
          //check that the caller has the lock
          hal_audio_write_lineout(buffer, size);
      }

      void playTone( const uint32_t freq, const uint32_t duractionInMS );

  private:
    os_mutex_t mutex_;
};
