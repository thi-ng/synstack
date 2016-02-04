#pragma once

#include "synth.h"

typedef struct {
    const float *src;
    const float *lfo;
    float pos;
} CT_PanningState;

CT_DSPNode *ct_synth_panning(char *id, CT_DSPNode *src, CT_DSPNode *lfo,
                             float pos);
uint8_t ct_synth_process_panning(CT_DSPNode *node, CT_DSPStack *stack,
                                 CT_Synth *synth, uint32_t offset);
