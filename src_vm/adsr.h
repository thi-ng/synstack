#pragma once

#include "synth_conf.h"
#include "vm.h"

typedef enum {
  IDLE    = 0,
  ATTACK  = 1,
  DECAY   = 2,
  SUSTAIN = 3,
  RELEASE = 4
} CTSS_ADSRPhase;

typedef struct {
  float currGain;
  float attackGain;
  float sustainGain;
  float attackRate;
  float decayRate;
  float releaseRate;
  CTSS_ADSRPhase phase;
} CTSS_ADSRState;

void ctss_init_adsr_ops(CTSS_VM *vm);
