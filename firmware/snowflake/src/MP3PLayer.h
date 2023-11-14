#pragma once

#include "Particle.h"
#include "AudioPlayer.h"

class MP3Player
{
  public:
      MP3Player( AudioPlayer* audioPlayer );

      void play( const String filename ) {
          String *filenamePtr = new String(filename);
          Log.info("MP3Player::play(%s)", filenamePtr->c_str());

          const int success = os_queue_put(queue_, &filenamePtr, CONCURRENT_WAIT_FOREVER, 0);
          if( 0 != success ) {
              Log.error("MP3Player::play(%s) failed to put song in queue", filenamePtr->c_str());
              delete filenamePtr;
          }
      }

  private:
    void internalPlaySong( const String filename );

    bool readMP3File( const String filename, uint8_t** mp3Data, uint32_t* mp3Size );

    //reference to the global audio output
    AudioPlayer* audioPlayer_;
    os_queue_t queue_;
    Thread* thread_;
};
