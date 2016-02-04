#pragma once

#include "synth.h"

#define PBLEP_OSC(name, expr)                                                  \
    uint8_t name(CT_DSPNode *node, CT_DSPStack *stack, CT_Synth *synth,        \
                 uint32_t offset) {                                            \
        CT_OscState *state = (CT_OscState *)node->state;                       \
        const float freq = state->freq * INV_TAU;                              \
        const float *lfo = state->lfo + offset;                                \
        const CT_PblepOsc fn = state->fn;                                      \
        float phase = state->phase;                                            \
        float *buf = node->buf + offset;                                       \
        uint32_t len = AUDIO_BUFFER_SIZE - offset;                             \
        while (len--) {                                                        \
            phase += freq;                                                     \
            phase -= (float)((int32_t)phase);                                  \
            *buf++ = ((expr)-ct_poly_blep(phase, freq)) * state->gain +        \
                     state->dcOffset;                                          \
        }                                                                      \
        state->phase = phase;                                                  \
        return 0;                                                              \
    }

typedef float (*CT_PblepOsc)(float t, const float dt, const float duty);

typedef struct {
    CT_PblepOsc fn;
    const float *lfo;
    const float *env;
    float lfoDepth;
    float envDepth;
    float pitchBend;
    float phase;
    float freq;
    float gain;
    float dcOffset;
} CT_OscState;

CT_DSPNode *ct_synth_osc(char *id, CT_DSPNodeHandler fn, float phase,
                         float freq, float gain, float dc);

uint8_t ct_synth_process_osc_sin(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_osc_square(CT_DSPNode *node, CT_DSPStack *stack,
                                    CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_osc_saw(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_osc_tri(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_osc_sawsin(CT_DSPNode *node, CT_DSPStack *stack,
                                    CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_osc_impulse(CT_DSPNode *node, CT_DSPStack *stack,
                                     CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_osc_pblep(CT_DSPNode *node, CT_DSPStack *stack,
                                   CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_osc_spiral(CT_DSPNode *node, CT_DSPStack *stack,
                                    CT_Synth *synth, uint32_t offset);

void ct_synth_set_osc_lfo(CT_DSPNode *node, const CT_DSPNode *lfo, float depth);
void ct_synth_set_osc_env(CT_DSPNode *node, const CT_DSPNode *env, float depth);
void ct_synth_set_osc_pblep(CT_DSPNode *node, CT_PblepOsc fn);

float ct_osc_pblep_saw(float t, const float dt, const float lfo);
float ct_osc_pblep_pwm(float t, const float dt, const float lfo);
float ct_osc_pblep_spiral(float t, const float dt, const float lfo);
