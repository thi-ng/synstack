#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct {
  char riffTag[4];
  uint32_t riffLength;
  char waveTag[4];
  char formatTag[4];
  uint32_t formatLength;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bits;
  char dataTag[4];
  uint32_t dataLength;
} CTSS_WavHeader;

uint8_t ctss_wavfile_save(const char *path,
                          const int16_t *data,
                          uint32_t rate,
                          uint8_t bits,
                          uint8_t channels,
                          uint32_t length);
FILE *ctss_wavfile_open(const char *path,
                        uint32_t rate,
                        uint8_t bits,
                        uint8_t channels);
void ctss_wavfile_write(FILE *file, const int16_t *data, uint32_t length);
void ctss_wavfile_close(FILE *file);
