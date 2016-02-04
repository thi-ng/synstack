#pragma once

#include "synth.h"

typedef struct {
    float *src;
    float threshold;
    float amp;
} CT_FoldbackState;

CT_DSPNode *ct_synth_foldback(char *id, CT_DSPNode *src, float threshold,
                              float amp);

uint8_t ct_synth_process_foldback(CT_DSPNode *node, CT_DSPStack *stack,
                                  CT_Synth *synth, uint32_t offset);
