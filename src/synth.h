#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "ctss_math.h"

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

#define CTSS_UNUSED(x) ((void)(x))

#define NODE_ID(p, id) ctss_node_for_id((p), (id))
#define NODE_ID_STATE(type, p, id) ((type *)(NODE_ID(p, id)->state))

typedef struct CTSS_DSPNode CTSS_DSPNode;
typedef struct CTSS_DSPStack CTSS_DSPStack;
typedef struct CTSS_Synth CTSS_Synth;

typedef uint8_t (*CTSS_DSPNodeHandler)(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                       CTSS_Synth *synth);

typedef void (*CTSS_Mixdown_I16)(float **sources, int16_t *out, uint32_t offset,
                                 uint32_t len, const uint8_t num,
                                 const uint8_t stride);

typedef void (*CTSS_Mixdown_F32)(float **sources, float *out, uint32_t offset,
                                 uint32_t len, const uint8_t num,
                                 const uint8_t stride);

typedef void *CTSS_DSPState;

typedef enum { STACK_ACTIVE = 1 } CTSS_DSPStackFlag;

typedef enum { NODE_ACTIVE = 1 } CTSS_DSPNodeFlag;

struct CTSS_DSPNode {
    float *buf;
    CTSS_DSPNodeHandler handler;
    CTSS_DSPState state;
    CTSS_DSPNode *next;
    char *id;
    uint8_t flags;
};

struct CTSS_DSPStack {
    CTSS_DSPNode *startNode;
    uint8_t flags;
};

struct CTSS_Synth {
    CTSS_DSPStack *stacks;
    CTSS_DSPStack post[1];
    CTSS_DSPNode *lfo[4];
    float **stackOutputs;
    uint8_t numStacks;
    uint8_t numLFO;
};

const float ctss_notes[96];
const float ctss_zero[AUDIO_BUFFER_SIZE];

void ctss_init(CTSS_Synth *synth, uint8_t numStacks);
void ctss_update(CTSS_Synth *synth);
void ctss_init_stack(CTSS_DSPStack *stack);
void ctss_build_stack(CTSS_DSPStack *stack, CTSS_DSPNode **nodes,
                      uint8_t length);
void ctss_collect_stacks(CTSS_Synth *synth);

void ctss_update_mix_mono_i16(CTSS_Synth *synth, CTSS_Mixdown_I16 mixdown,
                              uint32_t frames, int16_t *out);
void ctss_update_mix_stereo_i16(CTSS_Synth *synth, CTSS_Mixdown_I16 mixdown,
                                uint32_t frames, int16_t *out);
void ctss_update_mix_mono_f32(CTSS_Synth *synth, CTSS_Mixdown_F32 mixdown,
                              uint32_t frames, float *out);
void ctss_update_mix_stereo_f32(CTSS_Synth *synth, CTSS_Mixdown_F32 mixdown,
                                uint32_t frames, float *out);

void ctss_init_node(CTSS_DSPNode *node, char *id, uint8_t channels);
CTSS_DSPNode *ctss_node(char *id, uint8_t channels);
void ctss_free_node_state(CTSS_DSPNode *node);

void ctss_activate_stack(CTSS_DSPStack *stack);
void ctss_process_stack(CTSS_DSPStack *stack, CTSS_Synth *synth);
void ctss_stack_append(CTSS_DSPStack *stack, CTSS_DSPNode *node);
CTSS_DSPNode *ctss_stack_last_node(CTSS_DSPStack *stack);
CTSS_DSPNode *ctss_node_for_id(CTSS_DSPStack *stack, const char *id);

void ctss_trace_stack(CTSS_DSPStack *stack);
void ctss_trace_node(CTSS_DSPNode *node);

void ctss_mixdown_i16(float **in, int16_t *out, uint32_t offset, uint32_t len,
                      const uint8_t num, const uint8_t stride);
void ctss_mixdown_i16_3(float **in, int16_t *out, uint32_t offset, uint32_t len,
                        const uint8_t num, const uint8_t stride);
void ctss_mixdown_i16_4(float **in, int16_t *out, uint32_t offset, uint32_t len,
                        const uint8_t num, const uint8_t stride);
void ctss_mixdown_i16_5(float **in, int16_t *out, uint32_t offset, uint32_t len,
                        const uint8_t num, const uint8_t stride);
void ctss_mixdown_i16_6(float **in, int16_t *out, uint32_t offset, uint32_t len,
                        const uint8_t num, const uint8_t stride);

void ctss_mixdown_f32(float **sources, float *out, uint32_t offset,
                      uint32_t len, const uint8_t num, const uint8_t stride);
void ctss_mixdown_f32_3(float **sources, float *out, uint32_t offset,
                        uint32_t len, const uint8_t num, const uint8_t stride);
void ctss_mixdown_f32_4(float **sources, float *out, uint32_t offset,
                        uint32_t len, const uint8_t num, const uint8_t stride);
void ctss_mixdown_f32_5(float **sources, float *out, uint32_t offset,
                        uint32_t len, const uint8_t num, const uint8_t stride);
void ctss_mixdown_f32_6(float **sources, float *out, uint32_t offset,
                        uint32_t len, const uint8_t num, const uint8_t stride);
