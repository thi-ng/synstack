#include <math.h>
#include "osc.h"

CTSS_DSPNode *ctss_osc(char *id, CTSS_DSPNodeHandler fn, float phase,
                       float freq, float gain, float dc) {
    CTSS_DSPNode *node = ctss_node(id, 1);
    CTSS_OscState *osc = (CTSS_OscState *)calloc(1, sizeof(CTSS_OscState));
    osc->phase = phase;
    osc->freq = freq;
    osc->gain = gain;
    osc->dcOffset = dc;
    osc->lfo = ctss_zero;
    osc->env = ctss_zero;
    osc->lfoDepth = 0.0f;
    osc->envDepth = 0.0f;
    node->state = osc;
    node->handler = fn;
    return node;
}

void ctss_set_osc_lfo(CTSS_DSPNode *node, const CTSS_DSPNode *lfo,
                      float depth) {
    CTSS_OscState *state = (CTSS_OscState *)node->state;
    state->lfo = lfo->buf;
    state->lfoDepth = depth;
}

void ctss_set_osc_env(CTSS_DSPNode *node, const CTSS_DSPNode *env,
                      float depth) {
    CTSS_OscState *state = (CTSS_OscState *)node->state;
    state->env = env->buf;
    state->envDepth = depth;
}

void ctss_set_osc_pblep(CTSS_DSPNode *node, CTSS_PblepOsc fn) {
    ((CTSS_OscState *)node->state)->fn = fn;
}

uint8_t ctss_process_osc_sin(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                             CTSS_Synth *synth) {
    CTSS_UNUSED(synth);
    CTSS_UNUSED(stack);
    CTSS_OscState *state = (CTSS_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo;
    // const float *env = state->env;
    float phase = state->phase;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        *buf++ = state->dcOffset + ctss_fast_sin(phase) * state->gain;
    }
    state->phase = phase;
    return 0;
}

uint8_t ctss_process_osc_square(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                CTSS_Synth *synth) {
    CTSS_UNUSED(synth);
    CTSS_UNUSED(stack);
    CTSS_OscState *state = (CTSS_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo;
    // const float *env = state->env;
    float phase = state->phase;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        *buf++ = state->dcOffset + (phase < PI ? state->gain : -state->gain);
    }
    state->phase = phase;
    return 0;
}

uint8_t ctss_process_osc_saw(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                             CTSS_Synth *synth) {
    CTSS_UNUSED(synth);
    CTSS_UNUSED(stack);
    CTSS_OscState *state = (CTSS_OscState *)(node->state);
    const float freq = state->freq;
    const float *lfo = state->lfo;
    // const float *env = state->env;
    float phase = state->phase;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        *buf++ = state->dcOffset + (-phase * INV_PI + 1.0f) * state->gain;
    }
    state->phase = phase;
    return 0;
}

uint8_t ctss_process_osc_tri(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                             CTSS_Synth *synth) {
    CTSS_UNUSED(synth);
    CTSS_UNUSED(stack);
    CTSS_OscState *state = (CTSS_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo;
    // const float *env = state->env;
    float phase = state->phase;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        float x = 2.0f - (phase * INV_HALF_PI);
        x = 1.0f - ctss_stepf(x, 0.0f, -x, x);
        if (x > -1.0f) {
            *buf++ = x * state->gain + state->dcOffset;
        } else {
            *buf++ = state->dcOffset - state->gain;
        }
    }
    state->phase = phase;
    return 0;
}

uint8_t ctss_process_osc_sawsin(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                CTSS_Synth *synth) {
    CTSS_UNUSED(synth);
    CTSS_UNUSED(stack);
    CTSS_OscState *state = (CTSS_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo;
    // const float *env = state->env;
    float phase = state->phase;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        *buf++ = state->dcOffset +
                 ((phase > PI) ? -ctss_fast_sin(phase) : (phase * INV_PI - 1)) *
                     state->gain;
    }
    state->phase = phase;
    return 0;
}

uint8_t ctss_process_osc_impulse(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                 CTSS_Synth *synth) {
    CTSS_UNUSED(synth);
    CTSS_UNUSED(stack);
    CTSS_OscState *state = (CTSS_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo;
    // const float *env = state->env;
    float phase = state->phase;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        *buf++ = (state->gain * phase * expf(1.0f - phase)) + state->dcOffset;
    }
    state->phase = phase;
    return 0;
}

float ctss_osc_pblep_saw(float t, const float dt, const float lfo) {
    return 2.0f * t - 1.0f;
}

float ctss_osc_pblep_pwm(float t, const float dt, const float lfo) {
    return (t < lfo) ? -1.0 : 1.0f;
}

float ctss_osc_pblep_spiral(float t, const float dt, const float lfo) {
    return ctss_fast_cos(lfo * t * TAU) * t;
}

uint8_t ctss_process_osc_pblep(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                               CTSS_Synth *synth) {
    CTSS_UNUSED(synth);
    CTSS_UNUSED(stack);
    CTSS_OscState *state = (CTSS_OscState *)node->state;
    const float freq = state->freq * INV_TAU;
    const float *lfo = state->lfo;
    const CTSS_PblepOsc fn = state->fn;
    float phase = state->phase;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        phase += freq;
        // TRUNC_NORM(phase);
        phase -= (float)((int32_t)phase);
        *buf++ = (fn(phase, freq, *lfo++) - ctss_poly_blep(phase, freq)) *
                     state->gain +
                 state->dcOffset;
    }
    state->phase = phase;
    return 0;
}

PBLEP_OSC(ctss_process_osc_spiral, ctss_fast_cos(*lfo++ *phase *TAU) * phase);
