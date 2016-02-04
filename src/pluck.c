#include <string.h>
#include "pluck.h"

CT_DSPNode *ct_synth_osc_pluck(char *id, float freq, float impTime, float gain,
                               float dc) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_PluckOsc *osc = (CT_PluckOsc *)calloc(1, sizeof(CT_PluckOsc));
    osc->gain = gain;
    osc->dcOffset = dc;
    osc->variation = 0.0f;
    osc->damping = 1.0f;
    node->state = osc;
    node->handler = ct_synth_process_pluck;
    ct_synth_reset_pluck(node, freq, impTime, 0.5f);
    return node;
}

void ct_synth_reset_pluck(CT_DSPNode *node, float freq, float impTime,
                          float smooth) {
    if (freq < PLUCK_ACC_FREQ_LIMIT) {
        freq = PLUCK_ACC_FREQ_LIMIT;
    }
    CT_PluckOsc *state = (CT_PluckOsc *)node->state;
    memset((void *)state->acc, 0, sizeof(float) * state->len);
    state->phase = 1;
    state->impulse = (int32_t)(impTime * SAMPLE_RATE) - 1;
    state->len = (uint16_t)((float)SAMPLE_RATE / freq + 0.5f);
    state->smoothA = smooth;
    state->smoothB = 1.0 - smooth;
    state->lastImp = 0.0f;
}

uint8_t ct_synth_process_pluck(CT_DSPNode *node, CT_DSPStack *stack,
                               CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_PluckOsc *state = (CT_PluckOsc *)node->state;
    float *acc = state->acc;
    float *buf = node->buf + offset;
    int32_t impulse = state->impulse;
    uint16_t n = state->phase;
    const uint16_t alen = state->len;
    const float ca = state->smoothA;
    const float cb = state->smoothB;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        float xn;
        if (impulse >= 0) {
            impulse--;
            xn = (ct_normrandf() * (1.0f - state->variation) +
                  ct_normrandf() * state->variation) *
                 state->gain;
            state->lastImp += (xn - state->lastImp) * state->damping;
            xn = state->lastImp;
        } else {
            xn = 0.0f;
        }
        uint32_t n1 = (n + 1) % alen;
        acc[n] = xn + acc[n] * ca + acc[n1] * cb;
        *buf++ = acc[n] + state->dcOffset;
        n = n1;
    }
    state->phase = n;
    state->impulse = impulse;
    return 0;
}
