#pragma once

#include "synth.h"

#define FORMANT_TABLE_LEN 257
#define FORMANT_WIDTH 64

typedef enum {
    VOWEL_A = 0,
    VOWEL_E = 1,
    VOWEL_I = 2,
    VOWEL_O = 3,
    VOWEL_U = 4
} CT_Formant;

typedef struct {
    const float *src;
    double f[10];
    CT_Formant type;
} CT_FormantState;

typedef struct {
    float *coeff;
    float phase;
    float freq;
    float gain;
    float dcOffset;
    float smooth;
    float f[8];
} CT_FormantOsc;

void ct_synth_preinit_osc_formant(void);
CT_DSPNode *ct_synth_filter_formant(char *id, CT_Formant type, CT_DSPNode *src);

uint8_t ct_synth_process_formant(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset);

CT_DSPNode *ct_synth_osc_formant_id(char *id, uint8_t formant, float freq,
                                    float gain, float dc, float smooth);

CT_DSPNode *ct_synth_osc_formant(char *id, float *formant, float freq,
                                 float gain, float dc, float smooth);

uint8_t ct_synth_process_osc_formant(CT_DSPNode *node, CT_DSPStack *stack,
                                     CT_Synth *synth, uint32_t offset);

void ct_synth_set_formant_id(CT_DSPNode *node, uint8_t id);
