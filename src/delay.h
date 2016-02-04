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
} CTSS_DelayState;

CTSS_DSPNode *ctss_delay(char *id, CTSS_DSPNode *src, uint32_t len,
                         float feedback, uint8_t channels);

uint8_t ctss_process_delay(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                           CTSS_Synth *synth);
