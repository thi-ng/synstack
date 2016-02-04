#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "ct_math.h"

#ifndef AUDIO_BUFFER_SIZE
#define AUDIO_BUFFER_SIZE 32
#endif
#define AUDIO_BUFFER_SIZE2 (AUDIO_BUFFER_SIZE << 1)
#define AUDIO_BUFFER_SIZE4 (AUDIO_BUFFER_SIZE << 2)

#ifndef SAMPLE_RATE
#define SAMPLE_RATE 44100
#endif

#define INV_SAMPLE_RATE (1.0f / (float)SAMPLE_RATE)
#define NYQUIST_FREQ (SAMPLE_RATE >> 1)
#define INV_NYQUIST_FREQ (1.0f / (float)NYQUIST_FREQ)
#define TAU_RATE (TAU / (float)SAMPLE_RATE)
#define INV_TAU_RATE ((float)SAMPLE_RATE / TAU)

#define HZ_TO_RAD(freq) ((freq)*TAU_RATE)
#define RAD_TO_HZ(freq) ((freq)*INV_TAU_RATE)
#define TIME_TO_FS_RATE(t) (1.0f / (SAMPLE_RATE * (t)))
#define TRUNC_PHASE(phase)                                                     \
    if (phase >= TAU) {                                                        \
        phase -= TAU;                                                          \
    } else if (phase < 0.0) {                                                  \
        phase += TAU;                                                          \
    }

#define TRUNC_NORM(x)                                                          \
    if (x >= 1.0f) {                                                           \
        x -= 1.0f;                                                             \
    } else if (x < 0.0f) {                                                     \
        x += 1.0f;                                                             \
    }

#define CT_UNUSED(x) ((void)(x))

#define NODE_ID(p, id) ct_synth_node_for_id((p), (id))
#define NODE_ID_STATE(type, p, id) ((type *)(NODE_ID(p, id)->state))

typedef struct CT_DSPNode CT_DSPNode;
typedef struct CT_DSPStack CT_DSPStack;
typedef struct CT_Synth CT_Synth;

typedef uint8_t (*CT_DSPNodeHandler)(CT_DSPNode *node, CT_DSPStack *stack,
                                     CT_Synth *synth, uint32_t offset);

typedef void *CT_DSPState;

typedef enum { STACK_ACTIVE = 1 } CT_DSPStackFlag;

typedef enum { NODE_ACTIVE = 1 } CT_DSPNodeFlag;

struct CT_DSPNode {
    float *buf;
    CT_DSPNodeHandler handler;
    CT_DSPState state;
    CT_DSPNode *next;
    char *id;
    uint8_t flags;
};

struct CT_DSPStack {
    CT_DSPNode *startNode;
    uint8_t flags;
};

struct CT_Synth {
    CT_DSPStack *stacks;
    CT_DSPStack post[1];
    CT_DSPNode *lfo[4];
    float **stackOutputs;
    uint8_t numStacks;
    uint8_t numLFO;
};

const float ct_synth_notes[96];
const float ct_synth_zero[AUDIO_BUFFER_SIZE];

void ct_synth_init(CT_Synth *synth, uint8_t numStacks);
void ct_synth_update(CT_Synth *synth);
void ct_synth_init_stack(CT_DSPStack *stack);
void ct_synth_build_stack(CT_DSPStack *stack, CT_DSPNode **nodes,
                          uint8_t length);
void ct_synth_collect_stacks(CT_Synth *synth);

void ct_synth_update_mix_mono_i16(CT_Synth *synth, uint32_t frames,
                                  int16_t *out);
void ct_synth_update_mix_stereo_i16(CT_Synth *synth, uint32_t frames,
                                    int16_t *out);
void ct_synth_update_mix_mono_f32(CT_Synth *synth, uint32_t frames, float *out);
void ct_synth_update_mix_stereo_f32(CT_Synth *synth, uint32_t frames,
                                    float *out);

void ct_synth_init_node(CT_DSPNode *node, char *id, uint8_t channels);
CT_DSPNode *ct_synth_node(char *id, uint8_t channels);
void ct_synth_free_node_state(CT_DSPNode *node);

void ct_synth_activate_stack(CT_DSPStack *stack);
void ct_synth_process_stack(CT_DSPStack *stack, CT_Synth *synth,
                            uint32_t offset);
void ct_synth_stack_append(CT_DSPStack *stack, CT_DSPNode *node);
CT_DSPNode *ct_synth_stack_last_node(CT_DSPStack *stack);
CT_DSPNode *ct_synth_node_for_id(CT_DSPStack *stack, const char *id);

void ct_synth_trace_stack(CT_DSPStack *stack);
void ct_synth_trace_node(CT_DSPNode *node);

void ct_synth_mixdown_i16(float **in, int16_t *out, uint32_t offset,
                          uint32_t len, const uint8_t num,
                          const uint8_t stride);
void ct_synth_mixdown_f32(float **sources, float *out, uint32_t offset,
                          uint32_t len, const uint8_t num,
                          const uint8_t stride);
