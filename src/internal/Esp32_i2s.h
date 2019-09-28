#pragma once

#if defined(ARDUINO_ARCH_ESP32)

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#define I2S_DMA_MAX_DATA_LEN    4092// maximum bytes in one dma item

typedef enum {
    I2S_CHAN_STEREO, I2S_CHAN_RIGHT_TO_LEFT, I2S_CHAN_LEFT_TO_RIGHT, I2S_CHAN_RIGHT_ONLY, I2S_CHAN_LEFT_ONLY
} i2s_tx_chan_mod_t;

typedef enum {
    I2S_FIFO_16BIT_DUAL, I2S_FIFO_16BIT_SINGLE, I2S_FIFO_32BIT_DUAL, I2S_FIFO_32BIT_SINGLE
} i2s_tx_fifo_mod_t;

void i2sInit(uint8_t bus_num, uint32_t bits_per_sample, uint32_t sample_rate, i2s_tx_chan_mod_t chan_mod, i2s_tx_fifo_mod_t fifo_mod, size_t dma_count, size_t dma_len);

void i2sSetPins(uint8_t bus_num, int8_t out, int8_t ws, int8_t bck, int8_t in, bool invert);
void i2sSetDac(uint8_t bus_num, bool right, bool left);

esp_err_t i2sSetClock(uint8_t bus_num, uint8_t div_num, uint8_t div_b, uint8_t div_a, uint8_t bck, uint8_t bits_per_sample);
esp_err_t i2sSetSampleRate(uint8_t bus_num, uint32_t sample_rate, uint8_t bits_per_sample);

void i2sSetTxDataMode(uint8_t bus_num, i2s_tx_chan_mod_t chan_mod, i2s_tx_fifo_mod_t fifo_mod);

void i2sSetSilenceBuf(uint8_t bus_num, uint8_t* data, size_t len);

size_t i2sWrite(uint8_t bus_num, uint8_t* data, size_t len, bool copy, bool free_when_sent);
bool i2sWriteDone(uint8_t bus_num);

#ifdef __cplusplus
}
#endif

#endif
