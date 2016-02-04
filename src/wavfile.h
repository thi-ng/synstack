#pragma once

#include <stdio.h>
#include <stdint.h>

uint8_t ct_wavfile_save(const char *path, const int16_t *data, uint32_t rate,
                        uint8_t bits, uint8_t channels, uint32_t length);
FILE *ct_wavfile_open(const char *path, uint32_t rate, uint8_t bits,
                      uint8_t channels);
void ct_wavfile_write(FILE *file, const int16_t *data, uint32_t length);
void ct_wavfile_close(FILE *file);
