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
} CTSS_BiquadType;

typedef struct {
    float *src;
    float f[9]; // x1 = 5, x2 = 6, y1 = 7, y2 = 8
    CTSS_BiquadType type;
} CTSS_BiquadState;

CTSS_DSPNode *ctss_filter_biquad(char *id, CTSS_BiquadType type,
                                 CTSS_DSPNode *src, float freq, float dbGain,
                                 float bandwidth);
void ctss_calculate_biquad_coeff(CTSS_DSPNode *node, CTSS_BiquadType type,
                                 float freq, float dbGain, float bandwidth);
uint8_t ctss_process_biquad(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                            CTSS_Synth *synth);
