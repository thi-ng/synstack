#include "panning.h"
#include <math.h>

CTSS_DSPNode *ctss_panning(char *id,
                           CTSS_DSPNode *src,
                           CTSS_DSPNode *lfo,
                           float pos) {
  CTSS_DSPNode *node       = ctss_node(id, 2);
  CTSS_PanningState *state = calloc(1, sizeof(CTSS_PanningState));
  state->src               = src->buf;
  state->lfo               = (lfo != NULL ? lfo->buf : ctss_zero);
  state->pos               = pos;
  node->state              = state;
  node->handler            = ctss_process_panning;
  return node;
}

uint8_t ctss_process_panning(CTSS_DSPNode *node,
                             CTSS_DSPStack *stack,
                             CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_PanningState *state = node->state;
  const float *src         = state->src;
  const float pos          = state->pos + *(state->lfo);
  const float ampL         = sqrtf(1.0f - pos) * (CT_SQRT2 / 2.0f);
  const float ampR         = sqrtf(pos) * (CT_SQRT2 / 2.0f);
  float *buf               = node->buf;
  size_t len               = AUDIO_BUFFER_SIZE;
  while (len--) {
    *buf++ = *src * ampL;
    *buf++ = *src * ampR;
    src++;
  }
  return 0;
}
