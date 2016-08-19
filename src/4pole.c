#include "4pole.h"

CTSS_DSPNode *ctss_filter_4pole(char *id,
                                CTSS_DSPNode *src,
                                CTSS_DSPNode *lfo,
                                float freq,
                                float reso,
                                float coeff) {
  CTSS_DSPNode *node           = ctss_node(id, 1);
  CTSS_Filter4PoleState *state = CTSS_CALLOC(1, sizeof(CTSS_Filter4PoleState));
  state->src                   = &src->buf[0];
  state->lfo                   = (lfo != NULL ? &lfo->buf[0] : NULL);
  state->cutoffFreq            = freq;
  state->cutoffCoeff           = coeff;
  state->resonance             = reso;
  for (size_t i = 0; i < 4; i++) {
    state->in[i] = state->out[i] = 0.0f;
  }
  node->state   = state;
  node->handler = ctss_process_4pole;
  return node;
}

uint8_t ctss_process_4pole(CTSS_DSPNode *node,
                           CTSS_DSPStack *stack,
                           CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_Filter4PoleState *state = node->state;
  float *src                   = state->src;
  float *lfo                   = state->lfo;
  float *buf                   = &node->buf[0];
  float f0   = state->cutoffCoeff * state->cutoffFreq * INV_NYQUIST_FREQ;
  size_t len = AUDIO_BUFFER_SIZE;
  while (len--) {
    float f = f0;  // * (*lfo++);
    if (f > 1.0f) {
      f = 1.0f;
    }
    float ff  = f * f;
    float sig = *src++;
    sig -= state->out[3] * state->resonance * (1.0f - 0.15f * ff);
    sig *= 0.35013f * ff * ff;

    ff = 1.0f - f;

    state->out[0] = sig + 0.3f * state->in[0] + ff * state->out[0];
    state->out[1] = state->out[0] + 0.3f * state->in[1] + ff * state->out[1];
    state->out[2] = state->out[1] + 0.3f * state->in[2] + ff * state->out[2];
    *buf++        = state->out[3] =
        state->out[2] + 0.3f * state->in[3] + ff * state->out[3];

    state->in[0] = sig;
    state->in[1] = state->out[0];
    state->in[2] = state->out[1];
    state->in[3] = state->out[2];
  }
  return 0;
}
