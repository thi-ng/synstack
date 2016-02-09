#pragma once

#include "vm.h"
#include "synth_conf.h"

typedef struct { float phase; } CTSS_OscState;

void ctss_init_osc_ops(CTSS_VM *vm);
