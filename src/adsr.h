#pragma once

#include "synth.h"

typedef enum {
    IDLE = 0,
    ATTACK = 1,
    DECAY = 2,
    SUSTAIN = 3,
    RELEASE = 4
} CT_ADSRPhase;

typedef struct {
    float *lfo;
    float currGain;
    float attackGain;
    float sustainGain;
    float attackRate;
    float decayRate;
    float releaseRate;
    CT_ADSRPhase phase;
} CT_ADSRState;

CT_DSPNode *ct_synth_adsr(char *id, CT_DSPNode *lfo, float attTime,
                          float decayTime, float releaseTime, float attGain,
                          float sustainGain);
uint8_t ct_synth_process_adsr(CT_DSPNode *node, CT_DSPStack *stack,
                              CT_Synth *synth, uint32_t offset);
void ct_synth_configure_adsr(CT_DSPNode *node, float attTime, float decayTime,
                             float releaseTime, float attGain,
                             float sustainGain);
void ct_synth_reset_adsr(CT_DSPNode *node);
