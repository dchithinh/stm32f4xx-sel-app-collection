#ifndef AUDIO_CS43L22_H
#define AUDIO_CS43L22_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define AUDIO_PCM_FRAME_SAMPLES  2304U
#define AUDIO_PCM_HALVES         2U

extern I2S_HandleTypeDef hi2s3;
extern DMA_HandleTypeDef hdma_i2s3_tx;

int audio_codec_init(void);
void audio_codec_set_volume(uint8_t percent);
void audio_codec_play(void);
void audio_codec_pause(void);
void audio_codec_stop(void);
void audio_codec_mute(bool mute);

int audio_i2s_start(int16_t *pcm, uint32_t n_samples, uint32_t audio_hz);
void audio_i2s_stop(void);
bool audio_i2s_is_running(void);

void audio_pcm_mark_half(uint8_t half);
uint8_t audio_pcm_take_half(void);

#endif
