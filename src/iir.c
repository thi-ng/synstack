#include "iir.h"
#include <math.h>
#include <stdio.h>

CTSS_DSPNode *ctss_filter_iir(char *id,
                              CTSS_IIRType type,
                              CTSS_DSPNode *src,
                              CTSS_DSPNode *lfo,
                              float cutoff,
                              float reso) {
  CTSS_DSPNode *node   = ctss_node(id, 1);
  CTSS_IIRState *state = calloc(1, sizeof(CTSS_IIRState));
  state->src           = &src->buf[0];
  state->lfo           = (lfo != NULL ? &lfo->buf[0] : ctss_zero);
  state->type          = type;
  node->state          = state;
  node->handler        = ctss_process_iir;
  ctss_calculate_iir_coeff(node, cutoff, reso);
  return node;
}

void ctss_calculate_iir_coeff(CTSS_DSPNode *node, float cutoff, float reso) {
  CTSS_IIRState *state = node->state;
  state->cutoff        = cutoff;
  state->resonance     = reso;
  state->freq =
      2.0f * sinf(CT_PI * fminf(0.25f, cutoff / (SAMPLE_RATE * 2.0f)));
  state->damp = fminf(2.0f * (1.0f - powf(reso, 0.25f)),
                      fminf(2.0f, 2.0f / state->freq - state->freq * 0.5f));
  // printf("freq: %f, damp: %f\n", state->freq, state->damp);
  state->f[0] = 0.0f;  // lp
  state->f[1] = 0.0f;  // hp
  state->f[2] = 0.0f;  // bp
  state->f[3] = 0.0f;  // notch
}

uint8_t ctss_process_iir(CTSS_DSPNode *node,
                         CTSS_DSPStack *stack,
                         CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_IIRState *state = node->state;
  const float *src     = state->src;
  const float *lfo     = state->lfo;
  float *buf           = node->buf;
  float *f             = state->f;
  float damp           = state->damp;
  float freq           = state->freq;
  size_t len           = AUDIO_BUFFER_SIZE;
  while (len--) {
    float input = *src++;

    // 1st pass
    f[3] = input - damp * f[2];
    *f += freq * f[2];
    f[1] = f[3] - *f;
    f[2] += freq * f[1];
    float output = f[state->type];

    // 2nd pass
    f[3] = input - damp * f[2];
    *f += freq * f[2];
    f[1] = f[3] - *f;
    f[2] += freq * f[1];

    output = 0.5f * (output + f[state->type]);
    *buf++ = output;  //(input + (output - input) * *lfo++);
  }
  return 0;
}

// http://www.musicdsp.org/showArchiveComment.php?ArchiveID=235
static float cap = 0.0f;

float ctss_bassboost(float x,
                     const float sel,
                     const float amp,
                     const float wet) {
  float gain = 1.0f / (sel + 1.0f);
  cap        = (x + cap * sel) * gain;
  x          = (x + cap * wet) * amp;
  x          = (x < -1.0f) ? -1.0f : ((x > 1.0f) ? 1.0f : x);
  return x;
}
