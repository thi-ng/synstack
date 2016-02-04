#pragma once

#include "synth.h"

typedef enum {
    LPF,   /* low pass filter */
    HPF,   /* High pass filter */
    BPF,   /* band pass filter */
    NOTCH, /* Notch Filter */
    PEQ,   /* Peaking band EQ filter */
    LSH,   /* Low shelf filter */
    HSH    /* High shelf filter */
} CT_BiquadType;

typedef struct {
    float *src;
    float f[9]; // x1 = 5, x2 = 6, y1 = 7, y2 = 8
    CT_BiquadType type;
} CT_BiquadState;

CT_DSPNode *ct_synth_filter_biquad(char *id, CT_BiquadType type,
                                   CT_DSPNode *src, float freq, float dbGain,
                                   float bandwidth);
void ct_synth_calculate_biquad_coeff(CT_DSPNode *node, CT_BiquadType type,
                                     float freq, float dbGain, float bandwidth);
uint8_t ct_synth_process_biquad(CT_DSPNode *node, CT_DSPStack *stack,
                                CT_Synth *synth, uint32_t offset);
