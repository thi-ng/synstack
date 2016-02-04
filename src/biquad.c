#include <math.h>
#include "biquad.h"

/* sets up a BiQuad Filter */
CT_DSPNode *ct_synth_filter_biquad(char *id, CT_BiquadType type,
                                   CT_DSPNode *src, float freq, float dbGain,
                                   float bandwidth) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_BiquadState *state = (CT_BiquadState *)calloc(1, sizeof(CT_BiquadState));
    state->src = &src->buf[0];
    // state->lfo = (lfo != NULL ? &lfo->buf[0] : ct_synth_zero);
    state->type = type;
    node->state = state;
    node->handler = ct_synth_process_biquad;
    ct_synth_calculate_biquad_coeff(node, type, freq, dbGain, bandwidth);
    return node;
}

void ct_synth_calculate_biquad_coeff(CT_DSPNode *node, CT_BiquadType type,
                                     float freq, float dbGain,
                                     float bandwidth) {
    CT_BiquadState *state = (CT_BiquadState *)node->state;
    float a0, a1, a2, b0, b1, b2;

    float A = powf(10.0f, dbGain / 40.0f);
    float omega = HZ_TO_RAD(freq);
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn * sinh(LN2 / 2.0f * bandwidth * omega / sn);
    float beta = sqrtf(A + A);

    switch (type) {
    case LPF:
    default:
        b0 = (1.0f - cs) / 2.0f;
        b1 = 1.0f - cs;
        b2 = (1.0f - cs) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cs;
        a2 = 1.0f - alpha;
        break;
    case HPF:
        b0 = (1.0f + cs) / 2.0f;
        b1 = -(1.0f + cs);
        b2 = (1.0f + cs) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cs;
        a2 = 1.0f - alpha;
        break;
    case BPF:
        b0 = alpha;
        b1 = 0;
        b2 = -alpha;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cs;
        a2 = 1.0f - alpha;
        break;
    case NOTCH:
        b0 = 1.0f;
        b1 = -2.0f * cs;
        b2 = 1.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cs;
        a2 = 1.0f - alpha;
        break;
    case PEQ:
        b0 = 1.0f + (alpha * A);
        b1 = -2.0f * cs;
        b2 = 1.0f - (alpha * A);
        a0 = 1.0f + (alpha / A);
        a1 = -2.0f * cs;
        a2 = 1.0f - (alpha / A);
        break;
    case LSH:
        b0 = A * ((A + 1.0f) - (A - 1.0f) * cs + beta * sn);
        b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
        b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - beta * sn);
        a0 = (A + 1.0f) + (A - 1.0f) * cs + beta * sn;
        a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
        a2 = (A + 1.0f) + (A - 1.0f) * cs - beta * sn;
        break;
    case HSH:
        b0 = A * ((A + 1.0f) + (A - 1.0f) * cs + beta * sn);
        b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
        b2 = A * ((A + 1.0f) + (A - 1.0f) * cs - beta * sn);
        a0 = (A + 1.0f) - (A - 1.0f) * cs + beta * sn;
        a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
        a2 = (A + 1.0f) - (A - 1.0f) * cs - beta * sn;
        break;
    }

    a0 = 1.0f / a0;

    state->f[0] = b0 * a0;
    state->f[1] = b1 * a0;
    state->f[2] = b2 * a0;
    state->f[3] = a1 * a0;
    state->f[4] = a2 * a0;

    state->f[5] = state->f[6] = state->f[7] = state->f[8] = 0.0f;
}

uint8_t ct_synth_process_biquad(CT_DSPNode *node, CT_DSPStack *stack,
                                CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_BiquadState *state = (CT_BiquadState *)node->state;
    const float *src = state->src + offset;
    float *buf = node->buf + offset;
    float *f = state->f;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        float input = *src++;
        float x = f[0] * input + f[1] * f[5] + f[2] * f[6] - f[3] * f[7] -
                  f[4] * f[8];
        f[6] = f[5];
        f[5] = input;
        f[8] = f[7];
        f[7] = x;
        *buf++ = x;
    }
    return 0;
}
