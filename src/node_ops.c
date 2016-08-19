#include "node_ops.h"
#include <math.h>
#include "delay.h"

CTSS_DSPNode *ctss_op2(char *id,
                       CTSS_DSPNode *a,
                       CTSS_DSPNode *b,
                       CTSS_DSPNodeHandler fn) {
  CTSS_DSPNode *node       = ctss_node(id, 1);
  CTSS_NodeOp2State *state = CTSS_CALLOC(1, sizeof(CTSS_NodeOp2State));
  state->bufA              = a->buf;
  state->bufB              = b->buf;
  node->state              = state;
  node->handler            = fn;
  return node;
}

CTSS_DSPNode *ctss_op2_const(char *id,
                             CTSS_DSPNode *a,
                             float af,
                             CTSS_DSPNodeHandler fn) {
  CTSS_DSPNode *node        = ctss_node(id, 1);
  CTSS_NodeOp2CState *state = CTSS_CALLOC(1, sizeof(CTSS_NodeOp2CState));
  state->bufA               = a->buf;
  state->af                 = af;
  node->state               = state;
  node->handler             = fn;
  return node;
}

CTSS_DSPNode *ctss_op4_const(char *id,
                             CTSS_DSPNode *a,
                             float af,
                             CTSS_DSPNode *b,
                             float bf,
                             CTSS_DSPNodeHandler fn) {
  CTSS_DSPNode *node        = ctss_node(id, 1);
  CTSS_NodeOp4CState *state = CTSS_CALLOC(1, sizeof(CTSS_NodeOp4CState));
  state->bufA               = a->buf;
  state->af                 = af;
  state->bufB               = b->buf;
  state->bf                 = bf;
  node->state               = state;
  node->handler             = fn;
  return node;
}

CTSS_DSPNode *ctss_op4(char *id,
                       CTSS_DSPNode *a,
                       CTSS_DSPNode *b,
                       CTSS_DSPNode *c,
                       CTSS_DSPNode *d,
                       CTSS_DSPNodeHandler fn) {
  CTSS_DSPNode *node       = ctss_node(id, 1);
  CTSS_NodeOp4State *state = CTSS_CALLOC(1, sizeof(CTSS_NodeOp4State));
  state->bufA              = a->buf;
  state->bufB              = b->buf;
  state->bufC              = c->buf;
  state->bufD              = d->buf;
  node->state              = state;
  node->handler            = fn;
  return node;
}

CTSS_DSPNode *ctss_copy(char *id, CTSS_DSPNode *src, CTSS_DSPNode *dest) {
  CTSS_DSPNode *node       = ctss_node(id, 1);
  CTSS_NodeOp2State *state = CTSS_CALLOC(1, sizeof(CTSS_NodeOp2State));
  state->bufA              = src->buf;
  state->bufB              = dest->buf;
  node->state              = state;
  node->handler            = ctss_process_copy;
  return node;
}

uint8_t ctss_process_mult(CTSS_DSPNode *node,
                          CTSS_DSPStack *stack,
                          CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_NodeOp2State *state = node->state;
  const float *a           = state->bufA;
  float *b                 = state->bufB;
  float *buf               = node->buf;
  size_t len               = AUDIO_BUFFER_SIZE;
  while (len--) {
    *buf++ = (*a++) * (*b++);
  }
  return 0;
}

uint8_t ctss_process_sum(CTSS_DSPNode *node,
                         CTSS_DSPStack *stack,
                         CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_NodeOp2State *state = node->state;
  const float *a           = state->bufA;
  float *b                 = state->bufB;
  float *buf               = node->buf;
  size_t len               = AUDIO_BUFFER_SIZE;
  while (len--) {
    *buf++ = (*a++) + (*b++);
  }
  return 0;
}

uint8_t ctss_process_copy(CTSS_DSPNode *node,
                          CTSS_DSPStack *stack,
                          CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_NodeOp2State *state = node->state;
  const float *a           = state->bufA;
  float *b                 = state->bufB;
  float *buf               = node->buf;
  size_t len               = AUDIO_BUFFER_SIZE;
  while (len--) {
    *b     = *a++;
    *buf++ = *b++;
  }
  return 0;
}

uint8_t ctss_process_madd(CTSS_DSPNode *node,
                          CTSS_DSPStack *stack,
                          CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_NodeOp4State *state = node->state;
  const float *a           = state->bufA;
  const float *b           = state->bufB;
  const float *c           = state->bufC;
  const float *d           = state->bufD;
  float *buf               = node->buf;
  size_t len               = AUDIO_BUFFER_SIZE;
  while (len--) {
    *buf++ = ((*a++) * (*b++)) + ((*c++) * (*d++));
  }
  return 0;
}

uint8_t ctss_process_madd_const(CTSS_DSPNode *node,
                                CTSS_DSPStack *stack,
                                CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_NodeOp4CState *state = node->state;
  const float *a            = state->bufA;
  const float *b            = state->bufB;
  const float af            = state->af;
  const float bf            = state->bf;
  float *buf                = node->buf;
  size_t len                = AUDIO_BUFFER_SIZE;
  while (len--) {
    *buf++ = ((*a++) * af) + ((*b++) * bf);
  }
  return 0;
}
