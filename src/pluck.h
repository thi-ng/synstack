#pragma once

#include "synth.h"

#define PLUCK_ACC_LEN 0x600
#define PLUCK_ACC_FREQ_LIMIT ((float)SAMPLE_RATE / (float)PLUCK_ACC_LEN)

typedef struct {
    float acc[PLUCK_ACC_LEN]; // ~28.71 Hz lowest freq
    float gain;
    float dcOffset;
    float variation;
    float damping;
    float smoothA;
    float smoothB;
    float lastImp;
    int32_t impulse;
    uint32_t phase;
    uint16_t len;
} CTSS_PluckOsc;

CTSS_DSPNode *ctss_osc_pluck(char *id, float freq, float impTime, float gain,
                             float dc);
void ctss_reset_pluck(CTSS_DSPNode *node, float freq, float impTime,
                      float coeff);

uint8_t ctss_process_pluck(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                           CTSS_Synth *synth);
