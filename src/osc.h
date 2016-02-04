#pragma once

#include "synth.h"

#define PBLEP_OSC(name, expr)                                                  \
    uint8_t name(CTSS_DSPNode *node, CTSS_DSPStack *stack,                     \
                 CTSS_Synth *synth) {                                          \
        CTSS_OscState *state = (CTSS_OscState *)node->state;                   \
        const float freq = state->freq * INV_TAU;                              \
        const float *lfo = state->lfo;                                         \
        const CTSS_PblepOsc fn = state->fn;                                    \
        float phase = state->phase;                                            \
        float *buf = node->buf;                                                \
        uint32_t len = AUDIO_BUFFER_SIZE;                                      \
        while (len--) {                                                        \
            phase += freq;                                                     \
            phase -= (float)((int32_t)phase);                                  \
            *buf++ = ((expr)-ctss_poly_blep(phase, freq)) * state->gain +        \
                     state->dcOffset;                                          \
        }                                                                      \
        state->phase = phase;                                                  \
        return 0;                                                              \
    }

typedef float (*CTSS_PblepOsc)(float t, const float dt, const float duty);

typedef struct {
    CTSS_PblepOsc fn;
    const float *lfo;
    const float *env;
    float lfoDepth;
    float envDepth;
    float pitchBend;
    float phase;
    float freq;
    float gain;
    float dcOffset;
} CTSS_OscState;

CTSS_DSPNode *ctss_osc(char *id, CTSS_DSPNodeHandler fn, float phase,
                       float freq, float gain, float dc);

uint8_t ctss_process_osc_sin(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                             CTSS_Synth *synth);
uint8_t ctss_process_osc_square(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                CTSS_Synth *synth);
uint8_t ctss_process_osc_saw(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                             CTSS_Synth *synth);
uint8_t ctss_process_osc_tri(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                             CTSS_Synth *synth);
uint8_t ctss_process_osc_sawsin(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                CTSS_Synth *synth);
uint8_t ctss_process_osc_impulse(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                 CTSS_Synth *synth);
uint8_t ctss_process_osc_pblep(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                               CTSS_Synth *synth);
uint8_t ctss_process_osc_spiral(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                CTSS_Synth *synth);

void ctss_set_osc_lfo(CTSS_DSPNode *node, const CTSS_DSPNode *lfo, float depth);
void ctss_set_osc_env(CTSS_DSPNode *node, const CTSS_DSPNode *env, float depth);
void ctss_set_osc_pblep(CTSS_DSPNode *node, CTSS_PblepOsc fn);

float ctss_osc_pblep_saw(float t, const float dt, const float lfo);
float ctss_osc_pblep_pwm(float t, const float dt, const float lfo);
float ctss_osc_pblep_spiral(float t, const float dt, const float lfo);
