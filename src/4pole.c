#include "4pole.h"

CT_DSPNode *ct_synth_filter_4pole(char *id, CT_DSPNode *src, CT_DSPNode *lfo,
                                  float freq, float reso, float coeff) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_Filter4PoleState *state =
        (CT_Filter4PoleState *)calloc(1, sizeof(CT_Filter4PoleState));
    state->src = &src->buf[0];
    state->lfo = (lfo != NULL ? &lfo->buf[0] : NULL);
    state->cutoffFreq = freq;
    state->cutoffCoeff = coeff;
    state->resonance = reso;
    for (size_t i = 0; i < 4; i++) {
        state->in[i] = state->out[i] = 0.0f;
    }
    node->state = state;
    node->handler = ct_synth_process_filter4p;
    return node;
}

uint8_t ct_synth_process_filter4p(CT_DSPNode *node, CT_DSPStack *stack,
                                  CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_Filter4PoleState *state = (CT_Filter4PoleState *)(node->state);
    float *src = state->src + offset;
    float *lfo = state->lfo + offset;
    float *buf = &node->buf[0];
    float f0 = state->cutoffCoeff * state->cutoffFreq * INV_NYQUIST_FREQ;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        float f = f0; // * (*lfo++);
        if (f > 1.0f) {
            f = 1.0f;
        }
        float ff = f * f;
        float sig = (*src++);
        sig -= state->out[3] * state->resonance * (1.0f - 0.15f * ff);
        sig *= 0.35013f * ff * ff;

        ff = 1.0f - f;

        state->out[0] = sig + 0.3f * state->in[0] + ff * state->out[0];
        state->out[1] =
            state->out[0] + 0.3f * state->in[1] + ff * state->out[1];
        state->out[2] =
            state->out[1] + 0.3f * state->in[2] + ff * state->out[2];
        *buf++ = state->out[3] =
            state->out[2] + 0.3f * state->in[3] + ff * state->out[3];

        state->in[0] = sig;
        state->in[1] = state->out[0];
        state->in[2] = state->out[1];
        state->in[3] = state->out[2];
    }
    return 0;
}
