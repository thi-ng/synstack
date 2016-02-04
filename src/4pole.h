#pragma once

#include "synth.h"

typedef struct {
    float *src;
    float *lfo;
    float in[4];
    float out[4];
    float cutoffFreq;
    float cutoffCoeff;
    float resonance;
} CTSS_Filter4PoleState;

CTSS_DSPNode *ctss_filter_4pole(char *id, CTSS_DSPNode *src, CTSS_DSPNode *lfo,
                                float cutoff, float reso, float coeff);

uint8_t ctss_process_4pole(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                           CTSS_Synth *synth);
