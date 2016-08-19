#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "ct-head/log.h"
#include "wavfile.h"

uint8_t ctss_wavfile_save(const char *path,
                          const int16_t *data,
                          uint32_t rate,
                          uint8_t bits,
                          uint8_t channels,
                          uint32_t length) {
  FILE *f = ctss_wavfile_open(path, rate, bits, channels);
  if (!f) {
    CT_ERROR("couldn't open %s for writing: %s", path, strerror(errno));
    return 1;
  }
  ctss_wavfile_write(f, data, length);
  ctss_wavfile_close(f);
  return 0;
}

FILE *ctss_wavfile_open(const char *path,
                        uint32_t rate,
                        uint8_t bits,
                        uint8_t channels) {
  CTSS_WavHeader header = {.riffTag      = {'R', 'I', 'F', 'F'},
                           .waveTag      = {'W', 'A', 'V', 'E'},
                           .formatTag    = {'f', 'm', 't', ' '},
                           .dataTag      = {'d', 'a', 't', 'a'},
                           .riffLength   = 0,
                           .formatLength = 16,
                           .audioFormat  = 1,
                           .numChannels  = channels,
                           .sampleRate   = rate,
                           .byteRate     = rate * (bits >> 3),
                           .blockAlign   = bits >> 3,
                           .bits         = bits,
                           .dataLength   = 0};

  FILE *file = fopen(path, "w+");
  if (!file) {
    return NULL;
  }
  fwrite(&header, sizeof(header), 1, file);
  fflush(file);
  return file;
}

void ctss_wavfile_write(FILE *file, const int16_t *data, uint32_t length) {
  fwrite(data, sizeof(int16_t), length, file);
}

void ctss_wavfile_close(FILE *file) {
  uint32_t fLength = ftell(file);
  uint32_t rLength = fLength - 8;
  uint32_t dLength = fLength - sizeof(CTSS_WavHeader);
  fseek(file, offsetof(CTSS_WavHeader, dataLength), SEEK_SET);
  fwrite(&dLength, sizeof(dLength), 1, file);
  fseek(file, offsetof(CTSS_WavHeader, riffLength), SEEK_SET);
  fwrite(&rLength, sizeof(rLength), 1, file);
  fclose(file);
}
