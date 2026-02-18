#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <zephyr/kernel.h>

/**
 * I2S driver configuration constants
 */
#define SAMPLE_FREQUENCY 44100
#define SAMPLE_BIT_WIDTH 16
#define BYTES_PER_SAMPLE sizeof(int16_t)
#define NUMBER_OF_CHANNELS 1

#define TIMEOUT 1000

/**
 * Constants for configuring memory slab used as buffer for I2S driver
 */
// Want to generate a block of audio samples every 50 ms = 0.05 
#define BLOCK_GEN_PERIOD_MS 30

// How many samples we can produce within the block generation period given
// the sampling frequency
#define SAMPLES_PER_BLOCK                                                      \
  ((SAMPLE_FREQUENCY * BLOCK_GEN_PERIOD_MS / 1000) * NUMBER_OF_CHANNELS)

// how large the audio block can be to fill it within the block generation
// period
#define BLOCK_SIZE (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)

// the number of audio blocks to use
#define BLOCK_COUNT (2)

// Expose the audio slab for direct access (allocated in audio.cpp)
extern struct k_mem_slab mem_slab;

/// @brief Audio initialization function
/// Call this function before you call any other function from this library
/// @return 0 on success, -ERRNO otherwise
int initAudio();

/// @brief Set the output volume/amplitude of the audio amplifier
/// @param volumeValue volume value, from 0 to 255
/// @return 0 on success, -ERRNO otherwise
int setVolume(uint8_t volumeValue);

/// @brief Initialize the audio buffers
void *allocBlock();

/// @brief Write one of the buffers to the audio amplifier
/// Note: this is accomplished asynchronously through a DMA request
/// @param mem_block the audio buffer
/// @return 0 on success, -ERRNO otherwise
int writeBlock(void *mem_block);

void flushAudioBuffers();

#endif
