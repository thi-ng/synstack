#pragma once

#include "synth.h"

typedef struct {
    const float *bufA;
    float *bufB;
} CT_NodeOp2State;

typedef struct {
    const float *bufA;
    float af;
} CT_NodeOp2CState;

typedef struct {
    const float *bufA;
    const float *bufB;
    const float *bufC;
    const float *bufD;
} CT_NodeOp4State;

typedef struct {
    const float *bufA;
    const float *bufB;
    float af;
    float bf;
} CT_NodeOp4CState;

CT_DSPNode *ct_synth_op2(char *id, CT_DSPNode *a, CT_DSPNode *b,
                         CT_DSPNodeHandler fn);
CT_DSPNode *ct_synth_op2_const(char *id, CT_DSPNode *a, float af,
                               CT_DSPNodeHandler fn);

CT_DSPNode *ct_synth_op4(char *id, CT_DSPNode *a, CT_DSPNode *b, CT_DSPNode *c,
                         CT_DSPNode *d, CT_DSPNodeHandler fn);
CT_DSPNode *ct_synth_op4_const(char *id, CT_DSPNode *a, float af, CT_DSPNode *b,
                               float bf, CT_DSPNodeHandler fn);

CT_DSPNode *ct_synth_copy(char *id, CT_DSPNode *src, CT_DSPNode *dest);

uint8_t ct_synth_process_sum(CT_DSPNode *node, CT_DSPStack *stack,
                             CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_mult(CT_DSPNode *node, CT_DSPStack *stack,
                              CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_madd(CT_DSPNode *node, CT_DSPStack *stack,
                              CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_copy(CT_DSPNode *node, CT_DSPStack *stack,
                              CT_Synth *synth, uint32_t offset);
uint8_t ct_synth_process_madd_const(CT_DSPNode *node, CT_DSPStack *stack,
                                    CT_Synth *synth, uint32_t offset);
