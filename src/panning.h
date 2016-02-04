#pragma once

#include "synth.h"

typedef struct {
    const float *src;
    const float *lfo;
    float pos;
} CTSS_PanningState;

CTSS_DSPNode *ctss_panning(char *id, CTSS_DSPNode *src, CTSS_DSPNode *lfo,
                           float pos);
uint8_t ctss_process_panning(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                             CTSS_Synth *synth);
