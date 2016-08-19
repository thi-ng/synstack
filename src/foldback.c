#include "foldback.h"
#include <math.h>

CTSS_DSPNode *ctss_foldback(char *id,
                            CTSS_DSPNode *src,
                            float threshold,
                            float amp) {
  CTSS_DSPNode *node        = ctss_node(id, 1);
  CTSS_FoldbackState *state = CTSS_CALLOC(1, sizeof(CTSS_FoldbackState));
  state->src                = src->buf;
  state->threshold          = threshold;
  state->amp                = amp;
  node->state               = state;
  node->handler             = ctss_process_foldback;
  return node;
}

uint8_t ctss_process_foldback(CTSS_DSPNode *node,
                              CTSS_DSPStack *stack,
                              CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_FoldbackState *state = node->state;
  const float *src          = state->src;
  const float thresh        = state->threshold;
  const float thresh2       = thresh * 2.0f;
  const float thresh4       = thresh * 4.0f;
  const float amp           = state->amp;
  float *buf                = node->buf;
  size_t len                = AUDIO_BUFFER_SIZE;
  while (len--) {
    float in = *src++;
    if (in > thresh || in < -thresh) {
      in = (fabs(fabs(fmod(in - thresh, thresh4)) - thresh2) - thresh);
    }
    *buf++ = in * amp;
  }
  return 0;
}
