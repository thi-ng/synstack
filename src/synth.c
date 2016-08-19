#include <string.h>

#include "ct-head/log.h"
#include "synth.h"

void ctss_free_node_state(CTSS_DSPNode *node) {
  if (node->state != NULL) {
    CT_DEBUG("free node state...");
    CTSS_FREE(node->state);
    CTSS_FREE(node->id);
  }
}

void ctss_free_node(CTSS_DSPNode *node) {
  CT_DEBUG("free node: %s", node->id);
  ctss_free_node_state(node);
  CTSS_FREE(node);
}

void ctss_init(CTSS_Synth *synth, size_t numStacks) {
  memset((void *)ctss_zero, 0, sizeof(float) * AUDIO_BUFFER_SIZE);
  synth->stacks       = CTSS_CALLOC(numStacks, sizeof(CTSS_DSPStack));
  synth->stackOutputs = CTSS_CALLOC(numStacks, sizeof(float *));
  synth->numStacks    = numStacks;
  synth->numLFO       = 0;
}

void ctss_init_stack(CTSS_DSPStack *stack) {
  CTSS_DSPNode *node = stack->startNode;
  CTSS_DSPNode *next;
  while (node != NULL) {
    next = node->next;
    ctss_free_node(node);
    node = next;
  }
  stack->startNode = NULL;
  stack->flags     = 0;
}

void ctss_build_stack(CTSS_DSPStack *stack,
                      CTSS_DSPNode **nodes,
                      size_t length) {
  while (length--) {
    ctss_stack_append(stack, *nodes++);
  }
  ctss_activate_stack(stack);
}

void ctss_activate_stack(CTSS_DSPStack *stack) {
  stack->flags = STACK_ACTIVE;
}

void ctss_collect_stacks(CTSS_Synth *synth) {
  for (size_t i = 0; i < synth->numStacks; i++) {
    CTSS_DSPStack *s       = &synth->stacks[i];
    synth->stackOutputs[i] = ctss_stack_last_node(s)->buf;
    s->flags               = 0;
  }
}

int ctss_add_global_lfo(CTSS_Synth *synth, CTSS_DSPNode *lfo) {
  CT_CHECK(synth->numLFO < CTSS_MAX_LFO, "max LFOs reached: %d", CTSS_MAX_LFO);
  CT_DEBUG("add LFO: %s (%u)", lfo->id, synth->numLFO);
  synth->lfo[synth->numLFO] = lfo;
  synth->numLFO++;
  return 0;
fail:
  return 1;
}

void ctss_update_mix_mono_i16(CTSS_Synth *synth,
                              CTSS_Mixdown_I16 mixdown,
                              size_t frames,
                              int16_t *out) {
  for (size_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
    ctss_update(synth);
    mixdown(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE], 0,
            AUDIO_BUFFER_SIZE, synth->numStacks, 1);
  }
}

void ctss_update_mix_stereo_i16(CTSS_Synth *synth,
                                CTSS_Mixdown_I16 mixdown,
                                size_t frames,
                                int16_t *out) {
  for (size_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
    ctss_update(synth);
    mixdown(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2], 0,
            AUDIO_BUFFER_SIZE, synth->numStacks, 2);
    mixdown(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2 + 1], 1,
            AUDIO_BUFFER_SIZE, synth->numStacks, 2);
  }
}

void ctss_update_mix_mono_f32(CTSS_Synth *synth,
                              CTSS_Mixdown_F32 mixdown,
                              size_t frames,
                              float *out) {
  for (size_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
    ctss_update(synth);
    mixdown(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE], 0,
            AUDIO_BUFFER_SIZE, synth->numStacks, 1);
  }
}

void ctss_update_mix_stereo_f32(CTSS_Synth *synth,
                                CTSS_Mixdown_F32 mixdown,
                                size_t frames,
                                float *out) {
  for (size_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
    ctss_update(synth);
    mixdown(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2], 0,
            AUDIO_BUFFER_SIZE, synth->numStacks, 2);
    mixdown(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2 + 1], 1,
            AUDIO_BUFFER_SIZE, synth->numStacks, 2);
  }
}

CTSS_DSPNode *ctss_node(char *id, size_t channels) {
  CTSS_DSPNode *node = CTSS_CALLOC(1, sizeof(CTSS_DSPNode));
  ctss_init_node(node, id, channels);
  return node;
}

void ctss_init_node(CTSS_DSPNode *node, char *id, size_t channels) {
  if (node->buf != NULL) {
    CTSS_FREE(node->buf);
  }
  node->buf   = CTSS_CALLOC(AUDIO_BUFFER_SIZE * channels, sizeof(float));
  node->state = NULL;
  node->next  = NULL;
  node->flags = NODE_ACTIVE;
  node->id    = CTSS_CALLOC(strlen(id) + 1, sizeof(char));
  strcpy(node->id, id);
}

CTSS_DSPNode *ctss_stack_last_node(CTSS_DSPStack *stack) {
  CTSS_DSPNode *node = stack->startNode;
  while (node->next != NULL) {
    node = node->next;
  }
  return node;
}

CTSS_DSPNode *ctss_node_for_id(CTSS_DSPStack *stack, const char *id) {
  CTSS_DSPNode *node = stack->startNode;
  do {
    if (strcmp(id, node->id) == 0) {
      return node;
    }
    node = node->next;
  } while (node->next != NULL);
  return node;
}

void ctss_stack_append(CTSS_DSPStack *stack, CTSS_DSPNode *node) {
  if (stack->startNode == NULL) {
    stack->startNode = node;
  } else {
    ctss_stack_last_node(stack)->next = node;
  }
}

void ctss_process_stack(CTSS_DSPStack *stack, CTSS_Synth *synth) {
  if (stack->flags & STACK_ACTIVE) {
    CTSS_DSPNode *node = stack->startNode;
    size_t flags       = 0;
    while (1) {
      if (node->flags & NODE_ACTIVE) {
        flags |= node->handler(node, stack, synth);
        if (node->next == NULL) {
          break;
        }
      }
      node = node->next;
    }
    stack->flags &= (flags ^ 0xff);
  }
}

void ctss_trace_node(CTSS_DSPNode *node) {
  CT_DEBUG("-- %s --", node->id);
  for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
    CT_DEBUG("%03d: %f, ", i, node->buf[i]);
  }
}

void ctss_trace_stack(CTSS_DSPStack *stack) {
  CTSS_DSPNode *node = stack->startNode;
  while (1) {
    ctss_trace_node(node);
    if (node->next == NULL) {
      break;
    }
    node = node->next;
  }
}

void ctss_update(CTSS_Synth *synth) {
  for (size_t i = 0; i < synth->numLFO; i++) {
    synth->lfo[i]->handler(synth->lfo[i], NULL, synth);
  }
  CTSS_DSPStack *s = synth->stacks;
  for (size_t i = synth->numStacks; i > 0; i--, s++) {
    if (s->flags & STACK_ACTIVE) {
      ctss_process_stack(s, synth);
    }
  }
}

void ctss_mixdown_i16(float **sources,
                      int16_t *out,
                      size_t offset,
                      size_t len,
                      const size_t num,
                      const size_t stride) {
  while (len--) {
    float sum = 0;
    size_t n  = num;
    while (n--) {
      sum += *(sources[n] + offset);
    }
    *out = ct_clamp16((int32_t)(sum * 32767.0f));
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_i16_3(float **sources,
                        int16_t *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride) {
  while (len--) {
    float sum = *(sources[0] + offset);
    sum += *(sources[1] + offset);
    sum += *(sources[2] + offset);
    *out = ct_clamp16((int32_t)(sum * 32767.0f));
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_i16_4(float **sources,
                        int16_t *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride) {
  while (len--) {
    float sum = *(sources[0] + offset);
    sum += *(sources[1] + offset);
    sum += *(sources[2] + offset);
    sum += *(sources[3] + offset);
    *out = ct_clamp16((int32_t)(sum * 32767.0f));
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_i16_5(float **sources,
                        int16_t *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride) {
  while (len--) {
    float sum = *(sources[0] + offset);
    sum += *(sources[1] + offset);
    sum += *(sources[2] + offset);
    sum += *(sources[3] + offset);
    sum += *(sources[4] + offset);
    *out = ct_clamp16((int32_t)(sum * 32767.0f));
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_i16_6(float **sources,
                        int16_t *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride) {
  while (len--) {
    float sum = *(sources[0] + offset);
    sum += *(sources[1] + offset);
    sum += *(sources[2] + offset);
    sum += *(sources[3] + offset);
    sum += *(sources[4] + offset);
    sum += *(sources[5] + offset);
    *out = ct_clamp16((int32_t)(sum * 32767.0f));
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_f32(float **sources,
                      float *out,
                      size_t offset,
                      size_t len,
                      const size_t num,
                      const size_t stride) {
  while (len--) {
    float sum = 0;
    size_t n  = num;
    while (n--) {
      sum += *(sources[n] + offset);
    }
    if (sum < -1.0f) {
      sum = -1.0f;
    } else if (sum > 1.0f) {
      sum = 1.0f;
    }
    *out = sum;
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_f32_3(float **sources,
                        float *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride) {
  while (len--) {
    float sum = *(sources[0] + offset);
    sum += *(sources[1] + offset);
    sum += *(sources[2] + offset);
    if (sum < -1.0f) {
      sum = -1.0f;
    } else if (sum > 1.0f) {
      sum = 1.0f;
    }
    *out = sum;
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_f32_4(float **sources,
                        float *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride) {
  while (len--) {
    float sum = *(sources[0] + offset);
    sum += *(sources[1] + offset);
    sum += *(sources[2] + offset);
    sum += *(sources[3] + offset);
    if (sum < -1.0f) {
      sum = -1.0f;
    } else if (sum > 1.0f) {
      sum = 1.0f;
    }
    *out = sum;
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_f32_5(float **sources,
                        float *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride) {
  while (len--) {
    float sum = *(sources[0] + offset);
    sum += *(sources[1] + offset);
    sum += *(sources[2] + offset);
    sum += *(sources[3] + offset);
    sum += *(sources[4] + offset);
    if (sum < -1.0f) {
      sum = -1.0f;
    } else if (sum > 1.0f) {
      sum = 1.0f;
    }
    *out = sum;
    out += stride;
    offset += stride;
  }
}

void ctss_mixdown_f32_6(float **sources,
                        float *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride) {
  while (len--) {
    float sum = *(sources[0] + offset);
    sum += *(sources[1] + offset);
    sum += *(sources[2] + offset);
    sum += *(sources[3] + offset);
    sum += *(sources[4] + offset);
    sum += *(sources[5] + offset);
    if (sum < -1.0f) {
      sum = -1.0f;
    } else if (sum > 1.0f) {
      sum = 1.0f;
    }
    *out = sum;
    out += stride;
    offset += stride;
  }
}
