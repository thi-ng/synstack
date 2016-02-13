#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "vm.h"
#include "synth_conf.h"

#define PI 3.1415926535897932384626433832795029f
#define TAU (2.0f * PI)
#define HALF_PI (0.5f * PI)
#define INV_TAU (1.0f / TAU)
#define INV_PI (1.0f / PI)
#define INV_HALF_PI (1.0f / HALF_PI)

#define CTSS_TAU_RATE (TAU / (float)CTSS_SAMPLE_RATE)

#define CTSS_HZ_TO_RAD(freq) ((freq)*CTSS_TAU_RATE)

#define TIME_TO_FS_RATE(t) (1.0f / (CTSS_SAMPLE_RATE * (t)))

#define TRUNC_PHASE(phase)                                                     \
    if (phase >= TAU) {                                                        \
        phase -= TAU;                                                          \
    } else if (phase < 0.0) {                                                  \
        phase += TAU;                                                          \
    }

typedef struct {
    float buf[CTSS_AUDIO_BUFFER_SIZE];
    void *state;
} CTSS_SynthOp;

typedef struct { CTSS_VM vm; } CTSS_Synth;

#define CTSS_SYNTH_OP(ptr) (CTSS_SynthOp *)(*(ptr)).synthOp
#define CTSS_BUF(ptr) (CTSS_SYNTH_OP(ptr))->buf

#include "buffer.h"
#include "osc.h"
#include "adsr.h"
#include "foldback.h"
