#pragma once

#include "synth.h"

typedef enum { IIR_LP = 0, IIR_HP, IIR_BP, IIR_BR } IIRType;

typedef struct {
    const float *src;
    const float *lfo;
    float f[4];
    float cutoff;
    float resonance;
    float freq;
    float damp;
    IIRType type;
} CT_IIRState;

CT_DSPNode *ct_synth_filter_iir(char *id, IIRType type, CT_DSPNode *src,
                                CT_DSPNode *lfo, float cutoff, float reso);

void ct_synth_calculate_iir_coeff(CT_DSPNode *node, float cutoff, float reso);

uint8_t ct_synth_process_iir(CT_DSPNode *node, CT_DSPStack *stack,
                             CT_Synth *synth, uint32_t offset);

float ct_synth_bassboost(float x, const float sel, const float amp,
                         const float wet);
