#include <string.h>

#include "ct-head/log.h"
#include "pluck.h"

CTSS_DSPNode *ctss_osc_pluck(char *id,
                             float freq,
                             float impTime,
                             float gain,
                             float dc) {
  CTSS_DSPNode *node = ctss_node(id, 1);
  CTSS_PluckOsc *osc = CTSS_CALLOC(1, sizeof(CTSS_PluckOsc));
  osc->gain          = gain;
  osc->dcOffset      = dc;
  osc->variation     = 0.0f;
  osc->damping       = 1.0f;
  node->state        = osc;
  node->handler      = ctss_process_pluck;
  ct_xors_init(&osc->rnd);
  ctss_reset_pluck(node, freq, impTime, 0.5f);
  return node;
}

void ctss_reset_pluck(CTSS_DSPNode *node,
                      float freq,
                      float impTime,
                      float smooth) {
  if (freq < PLUCK_ACC_FREQ_LIMIT) {
    freq = PLUCK_ACC_FREQ_LIMIT;
  }
  CTSS_PluckOsc *state = node->state;
  memset((void *)state->acc, 0, sizeof(float) * state->len);
  state->phase   = 1;
  state->impulse = (int32_t)(impTime * SAMPLE_RATE) - 1;
  state->len     = (uint16_t)((float)SAMPLE_RATE / freq + 0.5f);
  state->smoothA = smooth;
  state->smoothB = 1.0f - smooth;
  state->lastImp = 0.0f;
}

uint8_t ctss_process_pluck(CTSS_DSPNode *node,
                           CTSS_DSPStack *stack,
                           CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_PluckOsc *state = node->state;
  float *acc           = state->acc;
  float *buf           = node->buf;
  int32_t impulse      = state->impulse;
  uint16_t n           = state->phase;
  const uint16_t alen  = state->len;
  const float ca       = state->smoothA;
  const float cb       = state->smoothB;
  size_t len           = AUDIO_BUFFER_SIZE;
  while (len--) {
    float xn;
    if (impulse >= 0) {
      impulse--;
      xn = (ct_xors_norm(&state->rnd) * (1.0f - state->variation) +
            ct_xors_norm(&state->rnd) * state->variation) *
           state->gain;
      state->lastImp += (xn - state->lastImp) * state->damping;
      xn = state->lastImp;
    } else {
      xn = 0.0f;
    }
    uint32_t n1 = (n + 1) % alen;
    acc[n]      = xn + acc[n] * ca + acc[n1] * cb;
    *buf++      = acc[n] + state->dcOffset;
    n           = n1;
  }
  state->phase   = n;
  state->impulse = impulse;
  return 0;
}
