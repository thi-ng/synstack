#pragma once

#include "vm.h"
#include "synth_conf.h"

typedef struct {
    float threshold;
    float amp;
} CTSS_FoldbackState;

void ctss_init_foldback_ops(CTSS_VM *vm);
