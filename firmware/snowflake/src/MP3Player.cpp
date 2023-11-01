#include "AudioPlayer.h"
#include "MP3Player.h"
#include "minimp3/minimp3.h"

MP3Player::MP3Player( AudioPlayer* audioPlayer )
  : audioPlayer_(audioPlayer)
{
    os_queue_create(&queue_, sizeof(String*), 1, NULL);

    thread_ = new Thread("mp3Player", [this]()->os_thread_return_t{

        while (1) {
            MP3PlayerQueueItem *item = NULL;

            if( 0 == os_queue_take(queue_, &item, CONCURRENT_WAIT_FOREVER, 0) ) {
               Log.info("MP3Player:: got song from queue: %s", item->filename->c_str());

                //if there is a callback, call it
                if( item->callback ) {
                    item->callback(true);
                }

                internalPlaySong(*(item->filename));

                //if there is a callback, call it
                if( item->callback ) {
                    item->callback(false);
                }

                delete item->filename;
                delete item;
            }
        }
    }, OS_THREAD_PRIORITY_NETWORK_HIGH, OS_THREAD_STACK_SIZE_DEFAULT_NETWORK );
}


void MP3Player::internalPlaySong( const String filename )
{
    Log.info("MP3Player::internalPlaySong(%s)", filename.c_str());

    if( 0 == audioPlayer_->aquireLock() )
    {
        audioPlayer_->setOutput(HAL_AUDIO_MODE_MONO, HAL_AUDIO_SAMPLE_RATE_16K, HAL_AUDIO_WORD_LEN_16);

        //load the mp3 from asset OTA into memory
        uint8_t* mp3Data = NULL;
        uint32_t mp3Size = 0;

        if( readMP3File( filename, &mp3Data, &mp3Size ) )
        {
            static mp3dec_t mp3d;
            mp3dec_init(&mp3d);

            mp3dec_frame_info_t info;
            static mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
            static mp3d_sample_t pcmFrames[MINIMP3_MAX_SAMPLES_PER_FRAME * 3]; // Max 3 frame (1152*2*2) in the worst case, but mp3 format that we used is usually 576 Bytes per frame
            static int pcmFramesLength = 0;

            //log we init the decoder
            Log.info("MP3Player::internalPlaySong(%s) init decoder", filename.c_str());

            int mp3len = 0;
            int samples = 0;

            do
            {
                samples = mp3dec_decode_frame(&mp3d, mp3Data + mp3len, mp3Size - mp3len, pcm, &info);
                if (samples > 0)
                {
                    //log we are playing
                    //Log.info("MP3Player::internalPlaySong(%s) playing %d samples", filename.c_str(), samples);

                    //trim the volume to 70% of current volume 
                    const float volume = 0.70;
                    for( int i = 0; i < samples; i++ )
                    {
                        //pcm[i] = pcm[i] * volume;
                    }

                    mp3len += info.frame_bytes;

                    memcpy(&pcmFrames[pcmFramesLength], pcm, samples*2);
                    pcmFramesLength += samples;
                    if (pcmFramesLength > 576 * 3) { // 3 frames
                        audioPlayer_->playBuffer((const uint16_t *)pcmFrames, pcmFramesLength*2);
                        pcmFramesLength = 0;
                    }
                }

            } while (samples > 0);

            //log that we finished
            Log.info("MP3Player::internalPlaySong(%s) finished", filename.c_str());

            //free the mp3 data
            free(mp3Data);
        }
        else
        {
            Log.error("MP3Player::internalPlaySong(%s) failed to read mp3 file", filename.c_str());
        }

        //terminate the audio output
        audioPlayer_->releaseLock();
    }
    else
    {
        Log.error("MP3Player::internalPlaySong(%s) failed to aquire audio lock", filename.c_str());
    }
}


bool MP3Player::readMP3File( const String filename, uint8_t** mp3Data, uint32_t* mp3Size )
{
    bool ok = false;

    Log.info("MP3Player::readMP3File(%s)", filename.c_str());

    // find the file in the assets
    auto assets = System.assetsAvailable();

    for (auto& asset: assets)
    {
        //print out the found asset
        Log.info("Found asset: %s", asset.name().c_str());

        if (asset.name() == filename)
        {
            Log.info("Found: %s size %d", filename.c_str(), asset.size() );

            //allocate memory for the mp3 data
            *mp3Data = (uint8_t*)malloc(asset.size());
            *mp3Size = asset.size();

            //read the file into memory. this function can return a lower numbr of bytes than requested and you have to call it in a loop until it returns 0
            uint32_t dataToRead = asset.size();
            uint32_t dataOffset = 0;
            do
            {
                int dataReadThisCycle = asset.read((char *)(*mp3Data+dataOffset), dataToRead);

                if( dataReadThisCycle < 0 )
                {
                    //error reading the file
                    break;
                }

                dataToRead -= dataReadThisCycle;
                dataOffset += dataReadThisCycle;

            } while ( dataToRead );

            if( dataToRead == 0 )
            {
                //success
                ok = true;
            }

            break;
        }
    }

    return ok;
}
