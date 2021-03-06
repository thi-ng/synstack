#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ct-head/math.h"
#include "ct-head/vargs.h"

#ifndef CTSS_CALLOC
#define CTSS_CALLOC calloc
#endif

#ifndef CTSS_FREE
#define CTSS_FREE free
#endif

#define DECLX(sym, ...) extern CT_CAT(sym, )(__VA_ARGS__);

DECLX(void *CTSS_CALLOC, size_t, size_t);
DECLX(void CTSS_FREE, void *);

#ifndef AUDIO_BUFFER_SIZE
#define AUDIO_BUFFER_SIZE 32
#endif
#define AUDIO_BUFFER_SIZE2 (AUDIO_BUFFER_SIZE << 1)
#define AUDIO_BUFFER_SIZE4 (AUDIO_BUFFER_SIZE << 2)

#ifndef SAMPLE_RATE
#define SAMPLE_RATE 44100
#endif

#ifndef CTSS_MAX_LFO
#define CTSS_MAX_LFO 4
#endif

#define INV_SAMPLE_RATE (1.0f / (float)SAMPLE_RATE)
#define NYQUIST_FREQ (SAMPLE_RATE >> 1)
#define INV_NYQUIST_FREQ (1.0f / (float)NYQUIST_FREQ)
#define TAU_RATE (CT_TAU / (float)SAMPLE_RATE)
#define INV_TAU_RATE ((float)SAMPLE_RATE / CT_TAU)

#define HZ_TO_RAD(freq) ((freq)*TAU_RATE)
#define RAD_TO_HZ(freq) ((freq)*INV_TAU_RATE)
#define TIME_TO_FS_RATE(t) (1.0f / (SAMPLE_RATE * (t)))
#define TRUNC_PHASE(phase) phase = ct_wrapf(phase, CT_TAU);
#define TRUNC_NORM(phase) phase  = ct_wrapf(phase, 1.0f);

#define CTSS_UNUSED(x) ((void)(x))

#define NODE_ID(p, id) ctss_node_for_id((p), (id))
#define NODE_ID_STATE(type, p, id) ((type *)(NODE_ID(p, id)->state))

typedef struct CTSS_DSPNode CTSS_DSPNode;
typedef struct CTSS_DSPStack CTSS_DSPStack;
typedef struct CTSS_Synth CTSS_Synth;

typedef uint8_t (*CTSS_DSPNodeHandler)(CTSS_DSPNode *node,
                                       CTSS_DSPStack *stack,
                                       CTSS_Synth *synth);

typedef void (*CTSS_Mixdown_I16)(float **sources,
                                 int16_t *out,
                                 size_t offset,
                                 size_t len,
                                 const size_t num,
                                 const size_t stride);

typedef void (*CTSS_Mixdown_F32)(float **sources,
                                 float *out,
                                 size_t offset,
                                 size_t len,
                                 const size_t num,
                                 const size_t stride);

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
  CTSS_DSPNode *lfo[CTSS_MAX_LFO];
  float **stackOutputs;
  uint8_t numStacks;
  uint8_t numLFO;
};

const float ctss_notes[96];
const float ctss_zero[AUDIO_BUFFER_SIZE];

void ctss_init(CTSS_Synth *synth, size_t numStacks);
void ctss_update(CTSS_Synth *synth);
void ctss_init_stack(CTSS_DSPStack *stack);
void ctss_build_stack(CTSS_DSPStack *stack,
                      CTSS_DSPNode **nodes,
                      size_t length);
void ctss_collect_stacks(CTSS_Synth *synth);
int ctss_add_global_lfo(CTSS_Synth *synth, CTSS_DSPNode *lfo);

void ctss_update_mix_mono_i16(CTSS_Synth *synth,
                              CTSS_Mixdown_I16 mixdown,
                              size_t frames,
                              int16_t *out);
void ctss_update_mix_stereo_i16(CTSS_Synth *synth,
                                CTSS_Mixdown_I16 mixdown,
                                size_t frames,
                                int16_t *out);
void ctss_update_mix_mono_f32(CTSS_Synth *synth,
                              CTSS_Mixdown_F32 mixdown,
                              size_t frames,
                              float *out);
void ctss_update_mix_stereo_f32(CTSS_Synth *synth,
                                CTSS_Mixdown_F32 mixdown,
                                size_t frames,
                                float *out);

void ctss_init_node(CTSS_DSPNode *node, char *id, size_t channels);
CTSS_DSPNode *ctss_node(char *id, size_t channels);
void ctss_free_node_state(CTSS_DSPNode *node);

void ctss_activate_stack(CTSS_DSPStack *stack);
void ctss_process_stack(CTSS_DSPStack *stack, CTSS_Synth *synth);
void ctss_stack_append(CTSS_DSPStack *stack, CTSS_DSPNode *node);
CTSS_DSPNode *ctss_stack_last_node(CTSS_DSPStack *stack);
CTSS_DSPNode *ctss_node_for_id(CTSS_DSPStack *stack, const char *id);

void ctss_trace_stack(CTSS_DSPStack *stack);
void ctss_trace_node(CTSS_DSPNode *node);

void ctss_mixdown_i16(float **in,
                      int16_t *out,
                      size_t offset,
                      size_t len,
                      const size_t num,
                      const size_t stride);
void ctss_mixdown_i16_3(float **in,
                        int16_t *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride);
void ctss_mixdown_i16_4(float **in,
                        int16_t *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride);
void ctss_mixdown_i16_5(float **in,
                        int16_t *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride);
void ctss_mixdown_i16_6(float **in,
                        int16_t *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride);

void ctss_mixdown_f32(float **sources,
                      float *out,
                      size_t offset,
                      size_t len,
                      const size_t num,
                      const size_t stride);
void ctss_mixdown_f32_3(float **sources,
                        float *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride);
void ctss_mixdown_f32_4(float **sources,
                        float *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride);
void ctss_mixdown_f32_5(float **sources,
                        float *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride);
void ctss_mixdown_f32_6(float **sources,
                        float *out,
                        size_t offset,
                        size_t len,
                        const size_t num,
                        const size_t stride);

// http://www.kvraudio.com/forum/viewtopic.php?t=375517

ct_inline float ctss_poly_blep(float t, const float dt) {
  // 0 <= t < 1
  if (t < dt) {
    t /= dt;
    // 2 * (t - t^2/2 - 0.5)
    return t + t - t * t - 1.f;
  }

  // -1 < t < 0
  else if (t > 1.f - dt) {
    t = (t - 1.0f) / dt;
    // 2 * (t^2/2 + t + 0.5)
    return t * t + t + t + 1.;
  }
  // 0 otherwise
  else {
    return 0.0f;
  }
}
