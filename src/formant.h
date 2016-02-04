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
} CTSS_Formant;

typedef struct {
    const float *src;
    double f[10];
    CTSS_Formant type;
} CTSS_FormantState;

typedef struct {
    float *coeff;
    float phase;
    float freq;
    float gain;
    float dcOffset;
    float smooth;
    float f[8];
} CTSS_FormantOsc;

void ctss_preinit_osc_formant(void);
CTSS_DSPNode *ctss_filter_formant(char *id, CTSS_Formant type,
                                  CTSS_DSPNode *src);

uint8_t ctss_process_formant(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                             CTSS_Synth *synth);

CTSS_DSPNode *ctss_osc_formant_id(char *id, uint8_t formant, float freq,
                                  float gain, float dc, float smooth);

CTSS_DSPNode *ctss_osc_formant(char *id, float *formant, float freq, float gain,
                               float dc, float smooth);

uint8_t ctss_process_osc_formant(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                 CTSS_Synth *synth);

void ctss_set_formant_id(CTSS_DSPNode *node, uint8_t id);
