#pragma once

#include "synth.h"

typedef enum {
  CTSS_IDLE    = 0,
  CTSS_ATTACK  = 1,
  CTSS_DECAY   = 2,
  CTSS_SUSTAIN = 3,
  CTSS_RELEASE = 4
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
  bool useSustain;
} CTSS_ADSRState;

CTSS_DSPNode *ctss_adsr(char *id, CTSS_DSPNode *lfo);

uint8_t ctss_process_adsr(CTSS_DSPNode *node,
                          CTSS_DSPStack *stack,
                          CTSS_Synth *synth);

void ctss_configure_adsr(CTSS_DSPNode *node,
                         float attTime,
                         float decayTime,
                         float releaseTime,
                         float attGain,
                         float sustainGain,
                         bool useSustain);

void ctss_reset_adsr(CTSS_DSPNode *node);

void ctss_release_adsr(CTSS_DSPNode *node);
