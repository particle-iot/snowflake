#pragma once

#include "Particle.h"
#include "AudioPlayer.h"

class MP3Player
{
  using MP3PlaybackCallback = std::function<void(const bool playing)>;

  typedef struct {
      String* filename;
      uint8_t volume;
      MP3PlaybackCallback callback;
  } MP3PlayerQueueItem;
  
  public:
      MP3Player( AudioPlayer* audioPlayer );

      void play( const String filename, const uint8_t volume = 100, MP3PlaybackCallback callback = nullptr ) {

          MP3PlayerQueueItem *item = new MP3PlayerQueueItem();
          item->filename = new String(filename);
          item->volume = volume;
          item->callback = callback;

          Log.info("MP3Player::play(%s)", item->filename->c_str());

          const int success = os_queue_put(queue_, &item, CONCURRENT_WAIT_FOREVER, 0);
          if( 0 != success ) {
              Log.error("MP3Player::play(%s) failed to put song in queue", item->filename->c_str());
              delete item->filename;
              delete item;
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
