#pragma once

#include "synth.h"

typedef struct {
    const float *bufA;
    float *bufB;
} CTSS_NodeOp2State;

typedef struct {
    const float *bufA;
    float af;
} CTSS_NodeOp2CState;

typedef struct {
    const float *bufA;
    const float *bufB;
    const float *bufC;
    const float *bufD;
} CTSS_NodeOp4State;

typedef struct {
    const float *bufA;
    const float *bufB;
    float af;
    float bf;
} CTSS_NodeOp4CState;

CTSS_DSPNode *ctss_op2(char *id, CTSS_DSPNode *a, CTSS_DSPNode *b,
                       CTSS_DSPNodeHandler fn);
CTSS_DSPNode *ctss_op2_const(char *id, CTSS_DSPNode *a, float af,
                             CTSS_DSPNodeHandler fn);

CTSS_DSPNode *ctss_op4(char *id, CTSS_DSPNode *a, CTSS_DSPNode *b,
                       CTSS_DSPNode *c, CTSS_DSPNode *d,
                       CTSS_DSPNodeHandler fn);
CTSS_DSPNode *ctss_op4_const(char *id, CTSS_DSPNode *a, float af,
                             CTSS_DSPNode *b, float bf, CTSS_DSPNodeHandler fn);

CTSS_DSPNode *ctss_copy(char *id, CTSS_DSPNode *src, CTSS_DSPNode *dest);

uint8_t ctss_process_sum(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                         CTSS_Synth *synth);
uint8_t ctss_process_mult(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                          CTSS_Synth *synth);
uint8_t ctss_process_madd(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                          CTSS_Synth *synth);
uint8_t ctss_process_copy(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                          CTSS_Synth *synth);
uint8_t ctss_process_madd_const(CTSS_DSPNode *node, CTSS_DSPStack *stack,
                                CTSS_Synth *synth);
