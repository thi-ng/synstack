#pragma once

#include "synth_conf.h"
#include "vm.h"

typedef struct {
  float threshold;
  float amp;
} CTSS_FoldbackState;

void ctss_init_foldback_ops(CTSS_VM *vm);
