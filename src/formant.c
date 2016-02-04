#include <math.h>
#include "formant.h"

// http://www.musicdsp.org/showArchiveComment.php?ArchiveID=110

static const double formant_filter_coeff[5 * 11] = {
    // A
    3.11044e-06, 8.943665402, -36.83889529, 92.01697887, -154.337906,
    181.6233289, -151.8651235, 89.09614114, -35.10298511, 8.388101016,
    -0.923313471,
    // E
    4.36215e-06, 8.90438318, -36.55179099, 91.05750846, -152.422234,
    179.1170248, -149.6496211, 87.78352223, -34.60687431, 8.282228154,
    -0.914150747,
    // I
    3.33819e-06, 8.893102966, -36.49532826, 90.96543286, -152.4545478,
    179.4835618, -150.315433, 88.43409371, -34.98612086, 8.407803364,
    -0.932568035,
    // O
    1.13572e-06, 8.994734087, -37.2084849, 93.22900521, -156.6929844,
    184.596544, -154.3755513, 90.49663749, -35.58964535, 8.478996281,
    -0.929252233,
    // U
    4.09431e-07, 8.997322763, -37.20218544, 93.11385476, -156.2530937,
    183.7080141, -153.2631681, 89.59539726, -35.12454591, 8.338655623,
    -0.910251753};

CT_DSPNode *ct_synth_filter_formant(char *id, CT_Formant formant,
                                    CT_DSPNode *src) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_FormantState *state =
        (CT_FormantState *)calloc(1, sizeof(CT_FormantState));
    state->src = src->buf;
    state->type = formant;
    node->state = state;
    node->handler = ct_synth_process_formant;
    return node;
}

uint8_t ct_synth_process_formant(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_FormantState *state = (CT_FormantState *)node->state;
    const float *src = state->src + offset;
    float *buf = node->buf + offset;
    double *coeff_ptr = &formant_filter_coeff[state->type * 11];
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        const double *coeff = coeff_ptr;
        double *f = state->f;
        double res = (*coeff++) * (double)(*src++);
        res += (*coeff++) * (*f++);
        res += (*coeff++) * (*f++);
        res += (*coeff++) * (*f++);
        res += (*coeff++) * (*f++);
        res += (*coeff++) * (*f++);
        res += (*coeff++) * (*f++);
        res += (*coeff++) * (*f++);
        res += (*coeff++) * (*f++);
        res += (*coeff++) * (*f++);
        res += *coeff * *f;

        double *f2 = f - 1;
        *f-- = *f2--;
        *f-- = *f2--;
        *f-- = *f2--;
        *f-- = *f2--;
        *f-- = *f2--;
        *f-- = *f2--;
        *f-- = *f2--;
        *f-- = *f2--;
        *f-- = *f2;
        *f = res;
        *buf++ = (float)res;
    }
    return 0;
}

static float formant_osc_fa[] = {
    // 0
    730, 1090, 2440, 3400, 1.0, 2.0, 0.3, 0.2,
    // 1
    200, 2100, 3100, 4700, 0.5, 0.5, 0.15, 0.1,
    // 2
    400, 900, 2300, 3000, 1.0, 0.7, 0.2, 0.2,
    // 3
    250, 1700, 2100, 3300, 1.0, 0.7, 0.4, 0.3,
    // 4
    190, 800, 2000, 3400, 0.7, 0.35, 0.1, 0.1,
    // 5
    350, 1900, 2500, 3700, 1.0, 0.3, 0.3, 0.1,
    // 6
    550, 1600, 2250, 3200, 1.0, 0.5, 0.7, 0.3,
    // 7
    550, 850, 1900, 3000, 0.3, 1.0, 0.2, 0.2,
    // 8
    450, 1100, 1500, 3000, 1.0, 0.7, 0.2, 0.3};

static float formant_osc_lut[FORMANT_TABLE_LEN * FORMANT_WIDTH];

static float calc_formant(const float p, const float I) {
    float a = 0.5f;
    int hmax = (int)(10 * I) > FORMANT_TABLE_LEN / 2 ? FORMANT_TABLE_LEN / 2
                                                     : (int)(10 * I);
    float ih = 1.0f / hmax;
    float phi = 0.0f;
    for (int h = 1; h < hmax; h++) {
        phi += PI * p;
        float hann = 0.5f + 0.5f * ct_norm_cos(h * ih);
        float gauss = 0.85f * expf(-h * h / (I * I));
        a += hann * (gauss + 0.15f) * cosf(phi);
    }
    return a;
}

static float lookup_formant(const float p, float w) {
    w = (w < 0) ? 0 : ((w > FORMANT_WIDTH - 2) ? (FORMANT_WIDTH - 2) : w);
    int p0 = (int)p;
    float fp = p - p0;
    int w0 = (int)w;
    float fw = w - w0;
    int i00 = p0 + FORMANT_TABLE_LEN * w0;
    int i10 = i00 + FORMANT_TABLE_LEN;
    // bilinear interpolation
    return (1 - fw) * (formant_osc_lut[i00] +
                       fp * (formant_osc_lut[i00 + 1] - formant_osc_lut[i00])) +
           fw * (formant_osc_lut[i10] +
                 fp * (formant_osc_lut[i10 + 1] - formant_osc_lut[i10]));
}

static float formant_carrier(const float h, const float p) {
    const float h0 = floorf(h);
    const float hf = h - h0;
    const float c0 = ct_norm_cos(fmodf(p * h0 + 1001.f, 2.0f) - 1.0f);
    const float c1 = ct_norm_cos(fmodf(p * (h0 + 1.0f) + 1001.f, 2.0f) - 1.0f);
    return c0 + hf * (c1 - c0);
}

void ct_synth_preinit_osc_formant(void) {
    const float coeff = 2.0f / (FORMANT_TABLE_LEN - 1);
    for (uint32_t i = 0; i < FORMANT_WIDTH; i++) {
        for (uint32_t p = 0; p < FORMANT_TABLE_LEN; p++) {
            formant_osc_lut[p + i * FORMANT_TABLE_LEN] =
                calc_formant(-1.0f + p * coeff, (float)i);
        }
    }
}

CT_DSPNode *ct_synth_osc_formant_id(char *id, uint8_t formant, float freq,
                                    float gain, float dc, float smooth) {
    return ct_synth_osc_formant(id, &formant_osc_fa[formant << 3], freq, gain,
                                dc, smooth);
}

CT_DSPNode *ct_synth_osc_formant(char *id, float *formant, float freq,
                                 float gain, float dc, float smooth) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_FormantOsc *osc = (CT_FormantOsc *)calloc(1, sizeof(CT_FormantOsc));
    osc->phase = -1.0f;
    osc->freq = freq;
    osc->gain = gain;
    osc->dcOffset = dc;
    osc->coeff = formant;
    osc->smooth = smooth;
    osc->f[0] = 100.0f;
    osc->f[1] = 100.0f;
    osc->f[2] = 100.0f;
    osc->f[3] = 100.0f;
    osc->f[4] = 0.0f;
    osc->f[5] = 0.0f;
    osc->f[6] = 0.0f;
    osc->f[7] = 0.0f;
    node->state = osc;
    node->handler = ct_synth_process_osc_formant;
    return node;
}

void ct_synth_set_formant_id(CT_DSPNode *node, uint8_t id) {
    CT_FormantOsc *state = (CT_FormantOsc *)(node->state);
    state->coeff = &formant_osc_fa[id << 3];
}

uint8_t ct_synth_process_osc_formant(CT_DSPNode *node, CT_DSPStack *stack,
                                     CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_FormantOsc *state = (CT_FormantOsc *)(node->state);
    const float freq = RAD_TO_HZ(state->freq);
    const float pf = freq * INV_NYQUIST_FREQ;
    const float invF = 1.0f / freq;
    const float smooth = state->smooth;
    const float *coeff = state->coeff;
    float *f = state->f;
    float phase = state->phase;
    float *buf = node->buf + offset;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    while (len--) {
        phase += pf;
        if (phase > 1.0f) {
            phase -= 2.0f;
        }
        float p = (FORMANT_TABLE_LEN - 1) * (phase + 1.0f) * 0.5f;

        f[0] += smooth * (coeff[0] - f[0]);
        f[1] += smooth * (coeff[1] - f[1]);
        f[2] += smooth * (coeff[2] - f[2]);
        f[3] += smooth * (coeff[3] - f[3]);
        f[4] += smooth * (coeff[4] - f[4]);
        f[5] += smooth * (coeff[5] - f[5]);
        f[6] += smooth * (coeff[6] - f[6]);
        f[7] += smooth * (coeff[7] - f[7]);

        float x = f[4] * (freq / f[0]) * lookup_formant(p, 100.0f * invF) *
                      formant_carrier(f[0] * invF, phase) +
                  f[5] * (freq / f[1]) * lookup_formant(p, 120.0f * invF) *
                      formant_carrier(f[1] * invF, phase) +
                  f[6] * (freq / f[2]) * lookup_formant(p, 150.0f * invF) *
                      formant_carrier(f[2] * invF, phase) +
                  f[7] * (freq / f[3]) * lookup_formant(p, 300.0f * invF) *
                      formant_carrier(f[3] * invF, phase);

        *buf++ = state->dcOffset + x * state->gain;
    }
    state->phase = phase;
    return 0;
}
