#include <stdio.h>
#include <string.h>
#include "synth.h"

void ct_synth_free_node_state(CT_DSPNode *node) {
    if (node->state != NULL) {
        printf("free node state...\n");
        free(node->state);
        free(node->id);
    }
}

void synth_free_node(CT_DSPNode *node) {
    printf("free node: %s\n", node->id);
    ct_synth_free_node_state(node);
    free(node);
}

void ct_synth_init(CT_Synth *synth, uint8_t numStacks) {
    memset((void *)ct_synth_zero, 0, sizeof(float) * AUDIO_BUFFER_SIZE);
    synth->stacks = (CT_DSPStack *)calloc(numStacks, sizeof(CT_DSPStack));
    synth->stackOutputs = (float **)calloc(numStacks, sizeof(float *));
    synth->numStacks = numStacks;
}

void ct_synth_init_stack(CT_DSPStack *stack) {
    CT_DSPNode *node = stack->startNode;
    CT_DSPNode *next;
    while (node != NULL) {
        next = node->next;
        synth_free_node(node);
        node = next;
    }
    stack->startNode = NULL;
    stack->flags = 0;
}

void ct_synth_build_stack(CT_DSPStack *stack, CT_DSPNode **nodes,
                          uint8_t length) {
    while (length--) {
        ct_synth_stack_append(stack, *nodes++);
    }
    ct_synth_activate_stack(stack);
}

void ct_synth_activate_stack(CT_DSPStack *stack) {
    stack->flags = STACK_ACTIVE;
}

void ct_synth_collect_stacks(CT_Synth *synth) {
    for (uint8_t i = 0; i < synth->numStacks; i++) {
        CT_DSPStack *s = &synth->stacks[i];
        synth->stackOutputs[i] = ct_synth_stack_last_node(s)->buf;
        s->flags = 0;
    }
}

void ct_synth_update_mix_mono_i16(CT_Synth *synth, uint32_t frames,
                                  int16_t *out) {
    for (uint32_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
        ct_synth_update(synth);
        ct_synth_mixdown_i16(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE],
                             0, AUDIO_BUFFER_SIZE, synth->numStacks, 1);
    }
}

void ct_synth_update_mix_stereo_i16(CT_Synth *synth, uint32_t frames,
                                    int16_t *out) {
    for (uint32_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
        ct_synth_update(synth);
        ct_synth_mixdown_i16(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2],
                             0, AUDIO_BUFFER_SIZE, synth->numStacks, 2);
        ct_synth_mixdown_i16(synth->stackOutputs,
                             &out[i * AUDIO_BUFFER_SIZE2 + 1], 1,
                             AUDIO_BUFFER_SIZE, synth->numStacks, 2);
    }
}

void ct_synth_update_mix_mono_f32(CT_Synth *synth, uint32_t frames,
                                  float *out) {
    for (uint32_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
        ct_synth_update(synth);
        ct_synth_mixdown_f32(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE],
                             0, AUDIO_BUFFER_SIZE, synth->numStacks, 1);
    }
}

void ct_synth_update_mix_stereo_f32(CT_Synth *synth, uint32_t frames,
                                    float *out) {
    for (uint32_t i = 0, num = frames / AUDIO_BUFFER_SIZE; i < num; i++) {
        ct_synth_update(synth);
        ct_synth_mixdown_f32(synth->stackOutputs, &out[i * AUDIO_BUFFER_SIZE2],
                             0, AUDIO_BUFFER_SIZE, synth->numStacks, 2);
        ct_synth_mixdown_f32(synth->stackOutputs,
                             &out[i * AUDIO_BUFFER_SIZE2 + 1], 1,
                             AUDIO_BUFFER_SIZE, synth->numStacks, 2);
    }
}

CT_DSPNode *ct_synth_node(char *id, uint8_t channels) {
    CT_DSPNode *node = (CT_DSPNode *)calloc(1, sizeof(CT_DSPNode));
    ct_synth_init_node(node, id, channels);
    return node;
}

void ct_synth_init_node(CT_DSPNode *node, char *id, uint8_t channels) {
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

CT_DSPNode *ct_synth_stack_last_node(CT_DSPStack *stack) {
    CT_DSPNode *node = stack->startNode;
    while (node->next != NULL) {
        node = node->next;
    }
    return node;
}

CT_DSPNode *ct_synth_node_for_id(CT_DSPStack *stack, const char *id) {
    CT_DSPNode *node = stack->startNode;
    do {
        if (strcmp(id, node->id) == 0) {
            return node;
        }
        node = node->next;
    } while (node->next != NULL);
    return node;
}

void ct_synth_stack_append(CT_DSPStack *stack, CT_DSPNode *node) {
    if (stack->startNode == NULL) {
        stack->startNode = node;
    } else {
        ct_synth_stack_last_node(stack)->next = node;
    }
}

void ct_synth_process_stack(CT_DSPStack *stack, CT_Synth *synth,
                            uint32_t offset) {
    if (stack->flags & STACK_ACTIVE) {
        CT_DSPNode *node = stack->startNode;
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

void ct_synth_trace_node(CT_DSPNode *node) {
    printf("-- %s --\n", node->id);
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        printf("%03d: %f, ", i, node->buf[i]);
    }
    printf("\n");
}

void ct_synth_trace_stack(CT_DSPStack *stack) {
    CT_DSPNode *node = stack->startNode;
    while (1) {
        ct_synth_trace_node(node);
        if (node->next == NULL) {
            break;
        }
        node = node->next;
    }
}

void ct_synth_update(CT_Synth *synth) {
    for (uint8_t i = 0; i < synth->numLFO; i++) {
        synth->lfo[i]->handler(synth->lfo[i], NULL, synth, 0);
    }
    CT_DSPStack *s = synth->stacks;
    for (uint8_t i = synth->numStacks; i > 0; i--, s++) {
        if (s->flags & STACK_ACTIVE) {
            ct_synth_process_stack(s, synth, 0);
        }
    }
}

void ct_synth_mixdown_i16(float **sources, int16_t *out, uint32_t offset,
                          uint32_t len, const uint8_t num,
                          const uint8_t stride) {
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

void ct_synth_mixdown_f32(float **sources, float *out, uint32_t offset,
                          uint32_t len, const uint8_t num,
                          const uint8_t stride) {
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
