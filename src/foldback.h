#pragma once

#include "synth.h"

typedef struct {
    float *src;
    float threshold;
    float amp;
} CTSS_FoldbackState;

CTSS_DSPNode *ctss_foldback(char *id, CTSS_DSPNode *src, float threshold,
                            float amp);

uint8_t ctss_process_foldback(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                              CTSS_Synth *synth);
