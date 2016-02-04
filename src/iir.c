#include <math.h>
#include <stdio.h>
#include "iir.h"

CT_DSPNode *ct_synth_filter_iir(char *id, IIRType type, CT_DSPNode *src,
                                CT_DSPNode *lfo, float cutoff, float reso) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_IIRState *state = (CT_IIRState *)calloc(1, sizeof(CT_IIRState));
    state->src = &src->buf[0];
    state->lfo = (lfo != NULL ? &lfo->buf[0] : ct_synth_zero);
    state->type = type;
    node->state = state;
    node->handler = ct_synth_process_iir;
    ct_synth_calculate_iir_coeff(node, cutoff, reso);
    return node;
}

void ct_synth_calculate_iir_coeff(CT_DSPNode *node, float cutoff, float reso) {
    CT_IIRState *state = (CT_IIRState *)node->state;
    state->cutoff = cutoff;
    state->resonance = reso;
    state->freq = 2.0f * sinf(PI * fminf(0.25f, cutoff / (SAMPLE_RATE * 2.0f)));
    state->damp = fminf(2.0f * (1.0f - powf(reso, 0.25f)),
                        fminf(2.0f, 2.0f / state->freq - state->freq * 0.5f));
    // printf("freq: %f, damp: %f\n", state->freq, state->damp);
    state->f[0] = 0.0f; // lp
    state->f[1] = 0.0f; // hp
    state->f[2] = 0.0f; // bp
    state->f[3] = 0.0f; // notch
}

uint8_t ct_synth_process_iir(CT_DSPNode *node, CT_DSPStack *stack,
                             CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_IIRState *state = (CT_IIRState *)node->state;
    const float *src = state->src + offset;
    const float *lfo = state->lfo + offset;
    float *buf = node->buf + offset;
    float *f = state->f;
    float damp = state->damp;
    float freq = state->freq;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        float input = *src++;

        // 1st pass
        f[3] = input - damp * f[2];
        *f += freq * f[2];
        f[1] = f[3] - *f;
        f[2] += freq * f[1];
        float output = f[state->type];

        // 2nd pass
        f[3] = input - damp * f[2];
        *f += freq * f[2];
        f[1] = f[3] - *f;
        f[2] += freq * f[1];

        output = 0.5f * (output + f[state->type]);
        *buf++ = output; //(input + (output - input) * *lfo++);
    }
    return 0;
}

// http://www.musicdsp.org/showArchiveComment.php?ArchiveID=235
static float cap = 0.0f;

float ct_synth_bassboost(float x, const float sel, const float amp,
                         const float wet) {
    float gain = 1.0f / (sel + 1.0f);
    cap = (x + cap * sel) * gain;
    x = (x + cap * wet) * amp;
    x = (x < -1.0f) ? -1.0f : ((x > 1.0f) ? 1.0f : x);
    return x;
}
