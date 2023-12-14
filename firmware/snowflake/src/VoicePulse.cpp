#include "VoicePulse.h"
#include "Sparkle_inferencing.h"

#define VP_DBG 0
#if VP_DBG
#define VP_DBG_INFO(fmt, ...) LOG(INFO, fmt "\r\n", ##__VA_ARGS__)
#define VP_DBG_PRINTF(fmt, ...) LOG(INFO, fmt, ##__VA_ARGS__)
#else
#define VP_DBG_INFO(fmt, ...)
#define VP_DBG_PRINTF(fmt, ...)
#endif

static constexpr size_t sampleBufferSize = (EI_CLASSIFIER_SLICE_SIZE * 2);

VoicePulse::VoicePulse(AudioPlayer* audioPlayer, VoicePulseDetectedCb callback, float threshold) :
                        audioPlayer_(audioPlayer),
                        callback_(callback),
                        threshold_(threshold) {
    // Allocate buffer for audio samples
    sampleBuffer_ = (int16_t*)malloc(sampleBufferSize * sizeof(int16_t));
    SPARK_ASSERT(sampleBuffer_ != nullptr);

}

void VoicePulse::start() {
    // Initialize the audio player
    audioPlayer_->setOutput(HAL_AUDIO_MODE_MONO, HAL_AUDIO_SAMPLE_RATE_16K, HAL_AUDIO_WORD_LEN_16);

    // Initialize the classifier
    run_classifier_init();

    // Create voice thread
    thread_ = new Thread("VoicePulse", [this]()->os_thread_return_t{

        // Summary of inferencing settings (from model_metadata.h)
        VP_DBG_PRINTF("Inferencing settings:");
        VP_DBG_PRINTF("\tInterval: %.2f ms.", (float)EI_CLASSIFIER_INTERVAL_MS);
        VP_DBG_PRINTF("\tFrame size: %d", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
        VP_DBG_PRINTF("\tSample length: %d ms.", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
        VP_DBG_PRINTF("\tNo. of classes: %d", sizeof(ei_classifier_inferencing_categories) /
                                                sizeof(ei_classifier_inferencing_categories[0]));

        while (1) {
            //LOG(INFO, "voicePulse thread running...");

            // Get a slice of audio data
            audioPlayer_->recordBuffer(sampleBuffer_, sampleBufferSize);

            VP_DBG_PRINTF("recordBuffer done");

            // Run classifier
            signal_t signal;
            signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
            signal.get_data = [this](size_t offset, size_t length, float *out_ptr) -> int {
                return this->microphone_audio_signal_get_data(offset, length, out_ptr);
            };

            VP_DBG_PRINTF("signal.get_data done");

            ei_impulse_result_t result = {0};
            EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, false);
            if (r != EI_IMPULSE_OK) {
                VP_DBG_PRINTF("ERR: Failed to run classifier (%d)\r\n", r);
                continue; 
            }

            VP_DBG_PRINTF("run_classifier_continuous done");

            if (++sliceCounter_ >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
                // print the predictions
                VP_DBG_PRINTF("Predictions ");
                VP_DBG_PRINTF("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
                    result.timing.dsp, result.timing.classification, result.timing.anomaly);
                VP_DBG_PRINTF(": \r\n");
                for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                    VP_DBG_PRINTF("    %s: %.5f\r\n", result.classification[ix].label,
                            result.classification[ix].value);
                }

                // Indicate the recognition result
                // classification,  0: "sparkle", 1: "unknown"
                if (result.classification[0].value > threshold_) {
                    callback_();
                }

        #if EI_CLASSIFIER_HAS_ANOMALY == 1
                VP_DBG_PRINTF("    anomaly score: %.3f\r\n", result.anomaly);
        #endif

                sliceCounter_ = 0;
            }

        }
    }, OS_THREAD_PRIORITY_NETWORK, OS_THREAD_STACK_SIZE_DEFAULT);
    SPARK_ASSERT(thread_ != nullptr);
}

int VoicePulse::microphone_audio_signal_get_data(size_t offset, size_t length, float* out_ptr) {
    numpy::int16_to_float(&sampleBuffer_[offset], out_ptr, length);
    return 0;
}
