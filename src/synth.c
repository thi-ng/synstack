#include <stdio.h>
#include <string.h>
#include "synth.h"

void ctss_free_node_state(CTSS_DSPNode *node) {
    if (node->state != NULL) {
        printf("free node state...\n");
        free(node->state);
        free(node->id);
    }
}

void ctss_free_node(CTSS_DSPNode *node) {
    printf("free node: %s\n", node->id);
    ctss_free_node_state(node);
    free(node);
}

void ctss_init(CTSS_Synth *synth, uint8_t numStacks) {
    memset((void *)ctss_zero, 0, sizeof(float) * AUDIO_BUFFER_SIZE);
    synth->stacks = (CTSS_DSPStack *)calloc(numStacks, sizeof(CTSS_DSPStack));
    synth->stackOutputs = (float **)calloc(numStacks, sizeof(float *));
    synth->numStacks = numStacks;
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
    stack->flags = 0;
}

void ctss_build_stack(CTSS_DSPStack *stack, CTSS_DSPNode **nodes,
                      uint8_t length) {
    while (length--) {
        ctss_stack_append(stack, *nodes++);
    }
    ctss_activate_stack(stack);
}

void ctss_activate_stack(CTSS_DSPStack *stack) {
    stack->flags = STACK_ACTIVE;
}

void ctss_collect_stacks(CTSS_Synth *synth) {
    for (uint8_t i = 0; i < synth->numStacks; i++) {
        CTSS_DSPStack *s = &synth->stacks[i];
        synth->stackOutputs[i] = ctss_stack_last_node(s)->buf;
        s->flags = 0;
    }
}

void ctss_update_mix_mono_i16(CTSS_Synth *synth, uint32_t frames,
                              int16_t *out) {
    for (uint32_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
        ctss_update(synth);
        ctss_mixdown_i16(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE], 0,
                         AUDIO_BUFFER_SIZE, synth->numStacks, 1);
    }
}

void ctss_update_mix_stereo_i16(CTSS_Synth *synth, uint32_t frames,
                                int16_t *out) {
    for (uint32_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
        ctss_update(synth);
        ctss_mixdown_i16(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2], 0,
                         AUDIO_BUFFER_SIZE, synth->numStacks, 2);
        ctss_mixdown_i16(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2 + 1],
                         1, AUDIO_BUFFER_SIZE, synth->numStacks, 2);
    }
}

void ctss_update_mix_mono_f32(CTSS_Synth *synth, uint32_t frames, float *out) {
    for (uint32_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
        ctss_update(synth);
        ctss_mixdown_f32(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE], 0,
                         AUDIO_BUFFER_SIZE, synth->numStacks, 1);
    }
}

void ctss_update_mix_stereo_f32(CTSS_Synth *synth, uint32_t frames,
                                float *out) {
    for (uint32_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
        ctss_update(synth);
        ctss_mixdown_f32(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2], 0,
                         AUDIO_BUFFER_SIZE, synth->numStacks, 2);
        ctss_mixdown_f32(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2 + 1],
                         1, AUDIO_BUFFER_SIZE, synth->numStacks, 2);
    }
}

CTSS_DSPNode *ctss_node(char *id, uint8_t channels) {
    CTSS_DSPNode *node = (CTSS_DSPNode *)calloc(1, sizeof(CTSS_DSPNode));
    ctss_init_node(node, id, channels);
    return node;
}

void ctss_init_node(CTSS_DSPNode *node, char *id, uint8_t channels) {
    if (node->buf != NULL) {
        free(node->buf);
    }
    node->buf = calloc(AUDIO_BUFFER_SIZE * channels, sizeof(float));
    node->state = NULL;
    node->next = NULL;
    node->flags = NODE_ACTIVE;
    node->id = (char *)calloc(strlen(id), sizeof(char));
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

void ctss_process_stack(CTSS_DSPStack *stack, CTSS_Synth *synth,
                        uint32_t offset) {
    if (stack->flags & STACK_ACTIVE) {
        CTSS_DSPNode *node = stack->startNode;
        uint8_t flags = 0;
        while (1) {
            if (node->flags & NODE_ACTIVE) {
                flags |= node->handler(node, stack, synth, offset);
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
    printf("-- %s --\n", node->id);
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        printf("%03d: %f, ", i, node->buf[i]);
    }
    printf("\n");
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
    for (uint8_t i = 0; i < synth->numLFO; i++) {
        synth->lfo[i]->handler(synth->lfo[i], NULL, synth, 0);
    }
    CTSS_DSPStack *s = synth->stacks;
    for (uint8_t i = synth->numStacks; i > 0; i--, s++) {
        if (s->flags & STACK_ACTIVE) {
            ctss_process_stack(s, synth, 0);
        }
    }
}

void ctss_mixdown_i16(float **sources, int16_t *out, uint32_t offset,
                      uint32_t len, const uint8_t num, const uint8_t stride) {
    while (len--) {
        float sum = 0;
        uint32_t n = num;
        while (n--) {
            sum += *(sources[n] + offset);
        }
        *out = ct_clamp16((int32_t)(sum * 32767.0f));
        out += stride;
        offset += stride;
    }
}

void ctss_mixdown_f32(float **sources, float *out, uint32_t offset,
                      uint32_t len, const uint8_t num, const uint8_t stride) {
    while (len--) {
        float sum = 0;
        uint32_t n = num;
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
