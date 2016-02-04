#pragma once

#include "synth.h"

typedef enum {
    IDLE = 0,
    ATTACK = 1,
    DECAY = 2,
    SUSTAIN = 3,
    RELEASE = 4
} CTSS_ADSRPhase;

typedef struct {
    float *lfo;
    float currGain;
    float attackGain;
    float sustainGain;
    float attackRate;
    float decayRate;
    float releaseRate;
    CTSS_ADSRPhase phase;
} CTSS_ADSRState;

CTSS_DSPNode *ctss_adsr(char *id, CTSS_DSPNode *lfo, float attTime,
                        float decayTime, float releaseTime, float attGain,
                        float sustainGain);
uint8_t ctss_process_adsr(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                          CTSS_Synth *synth);
void ctss_configure_adsr(CTSS_DSPNode *node, float attTime, float decayTime,
                         float releaseTime, float attGain, float sustainGain);
void ctss_reset_adsr(CTSS_DSPNode *node);
