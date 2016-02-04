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
} CT_Filter4PoleState;

CT_DSPNode *ct_synth_filter_4pole(char *id, CT_DSPNode *src, CT_DSPNode *lfo,
                                  float cutoff, float reso, float coeff);

uint8_t ct_synth_process_filter4p(CT_DSPNode *node, CT_DSPStack *stack,
                                  CT_Synth *synth, uint32_t offset);
