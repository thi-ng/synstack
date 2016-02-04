#include <math.h>
#include "osc.h"

CT_DSPNode *ct_synth_osc(char *id, CT_DSPNodeHandler fn, float phase,
                         float freq, float gain, float dc) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_OscState *osc = (CT_OscState *)calloc(1, sizeof(CT_OscState));
    osc->phase = phase;
    osc->freq = freq;
    osc->gain = gain;
    osc->dcOffset = dc;
    osc->lfo = ct_synth_zero;
    osc->env = ct_synth_zero;
    osc->lfoDepth = 0.0f;
    osc->envDepth = 0.0f;
    node->state = osc;
    node->handler = fn;
    return node;
}

void ct_synth_set_osc_lfo(CT_DSPNode *node, const CT_DSPNode *lfo,
                          float depth) {
    CT_OscState *state = (CT_OscState *)node->state;
    state->lfo = lfo->buf;
    state->lfoDepth = depth;
}

void ct_synth_set_osc_env(CT_DSPNode *node, const CT_DSPNode *env,
                          float depth) {
    CT_OscState *state = (CT_OscState *)node->state;
    state->env = env->buf;
    state->envDepth = depth;
}

void ct_synth_set_osc_pblep(CT_DSPNode *node, CT_PblepOsc fn) {
    ((CT_OscState *)node->state)->fn = fn;
}

uint8_t ct_synth_process_osc_sin(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_OscState *state = (CT_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo + offset;
    // const float *env = state->env + offset;
    float phase = state->phase;
    float *buf = node->buf + offset;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        *buf++ = state->dcOffset + ct_fast_sin(phase) * state->gain;
    }
    state->phase = phase;
    return 0;
}

uint8_t ct_synth_process_osc_square(CT_DSPNode *node, CT_DSPStack *stack,
                                    CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_OscState *state = (CT_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo + offset;
    // const float *env = state->env + offset;
    float phase = state->phase;
    float *buf = node->buf + offset;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        *buf++ = state->dcOffset + (phase < PI ? state->gain : -state->gain);
    }
    state->phase = phase;
    return 0;
}

uint8_t ct_synth_process_osc_saw(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_OscState *state = (CT_OscState *)(node->state);
    const float freq = state->freq;
    const float *lfo = state->lfo + offset;
    // const float *env = state->env + offset;
    float phase = state->phase;
    float *buf = node->buf + offset;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        *buf++ = state->dcOffset + (-phase * INV_PI + 1.0f) * state->gain;
    }
    state->phase = phase;
    return 0;
}

uint8_t ct_synth_process_osc_tri(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_OscState *state = (CT_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo + offset;
    // const float *env = state->env + offset;
    float phase = state->phase;
    float *buf = node->buf + offset;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        float x = 2.0f - (phase * INV_HALF_PI);
        x = 1.0f - ct_stepf(x, 0.0f, -x, x);
        if (x > -1.0f) {
            *buf++ = x * state->gain + state->dcOffset;
        } else {
            *buf++ = state->dcOffset - state->gain;
        }
    }
    state->phase = phase;
    return 0;
}

uint8_t ct_synth_process_osc_sawsin(CT_DSPNode *node, CT_DSPStack *stack,
                                    CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_OscState *state = (CT_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo + offset;
    // const float *env = state->env + offset;
    float phase = state->phase;
    float *buf = node->buf + offset;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        TRUNC_PHASE(phase);
        *buf++ = state->dcOffset +
                 ((phase > PI) ? -ct_fast_sin(phase) : (phase * INV_PI - 1)) *
                     state->gain;
    }
    state->phase = phase;
    return 0;
}

uint8_t ct_synth_process_osc_impulse(CT_DSPNode *node, CT_DSPStack *stack,
                                     CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_OscState *state = (CT_OscState *)node->state;
    const float freq = state->freq;
    const float *lfo = state->lfo + offset;
    // const float *env = state->env + offset;
    float phase = state->phase;
    float *buf = node->buf + offset;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        phase +=
            freq + (*lfo++ * state->lfoDepth); // + (*env++ * state->envDepth);
        *buf++ = (state->gain * phase * expf(1.0f - phase)) + state->dcOffset;
    }
    state->phase = phase;
    return 0;
}

float ct_osc_pblep_saw(float t, const float dt, const float lfo) {
    return 2.0f * t - 1.0f;
}

float ct_osc_pblep_pwm(float t, const float dt, const float lfo) {
    return (t < lfo) ? -1.0 : 1.0f;
}

float ct_osc_pblep_spiral(float t, const float dt, const float lfo) {
    return ct_fast_cos(lfo * t * TAU) * t;
}

uint8_t ct_synth_process_osc_pblep(CT_DSPNode *node, CT_DSPStack *stack,
                                   CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_OscState *state = (CT_OscState *)node->state;
    const float freq = state->freq * INV_TAU;
    const float *lfo = state->lfo + offset;
    const CT_PblepOsc fn = state->fn;
    float phase = state->phase;
    float *buf = node->buf + offset;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        phase += freq;
        // TRUNC_NORM(phase);
        phase -= (float)((int32_t)phase);
        *buf++ = (fn(phase, freq, *lfo++) - ct_poly_blep(phase, freq)) *
                     state->gain +
                 state->dcOffset;
    }
    state->phase = phase;
    return 0;
}

PBLEP_OSC(ct_synth_process_osc_spiral, ct_fast_cos(*lfo++ *phase *TAU) * phase);
