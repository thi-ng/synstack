#pragma once

#include <stdio.h>
#include <stdint.h>

uint8_t ctss_wavfile_save(const char *path, const int16_t *data, uint32_t rate,
                          uint8_t bits, uint8_t channels, uint32_t length);
FILE *ctss_wavfile_open(const char *path, uint32_t rate, uint8_t bits,
                        uint8_t channels);
void ctss_wavfile_write(FILE *file, const int16_t *data, uint32_t length);
void ctss_wavfile_close(FILE *file);
