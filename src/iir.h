#pragma once

#include "synth.h"

typedef enum { IIR_LP = 0, IIR_HP, IIR_BP, IIR_BR } CTSS_IIRType;

typedef struct {
    const float *src;
    const float *lfo;
    float f[4];
    float cutoff;
    float resonance;
    float freq;
    float damp;
    CTSS_IIRType type;
} CTSS_IIRState;

CTSS_DSPNode *ctss_filter_iir(char *id, CTSS_IIRType type, CTSS_DSPNode *src,
                              CTSS_DSPNode *lfo, float cutoff, float reso);

void ctss_calculate_iir_coeff(CTSS_DSPNode *node, float cutoff, float reso);

uint8_t ctss_process_iir(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                         CTSS_Synth *synth);

float ctss_bassboost(float x, const float sel, const float amp,
                     const float wet);
