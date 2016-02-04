#pragma once

#include "synth.h"

typedef struct {
    float *src;
    float *delayLine;
    float *readPtr;
    float *writePtr;
    uint32_t readPos;
    uint32_t writePos;
    uint32_t len;
    float feedback;
    uint8_t channels;
} CT_DelayState;

CT_DSPNode *ct_synth_delay(char *id, CT_DSPNode *src, uint32_t len,
                           float feedback, uint8_t channels);

uint8_t ct_synth_process_delay(CT_DSPNode *node, CT_DSPStack *stack,
                               CT_Synth *synth, uint32_t offset);
