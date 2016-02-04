#include <math.h>
#include "node_ops.h"
#include "delay.h"

CT_DSPNode *ct_synth_op2(char *id, CT_DSPNode *a, CT_DSPNode *b,
                         CT_DSPNodeHandler fn) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_NodeOp2State *state =
        (CT_NodeOp2State *)calloc(1, sizeof(CT_NodeOp2State));
    state->bufA = a->buf;
    state->bufB = b->buf;
    node->state = state;
    node->handler = fn;
    return node;
}

CT_DSPNode *ct_synth_op2_const(char *id, CT_DSPNode *a, float af,
                               CT_DSPNodeHandler fn) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_NodeOp2CState *state =
        (CT_NodeOp2CState *)calloc(1, sizeof(CT_NodeOp2CState));
    state->bufA = a->buf;
    state->af = af;
    node->state = state;
    node->handler = fn;
    return node;
}

CT_DSPNode *ct_synth_op4_const(char *id, CT_DSPNode *a, float af, CT_DSPNode *b,
                               float bf, CT_DSPNodeHandler fn) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_NodeOp4CState *state =
        (CT_NodeOp4CState *)calloc(1, sizeof(CT_NodeOp4CState));
    state->bufA = a->buf;
    state->af = af;
    state->bufB = b->buf;
    state->bf = bf;
    node->state = state;
    node->handler = fn;
    return node;
}

CT_DSPNode *ct_synth_op4(char *id, CT_DSPNode *a, CT_DSPNode *b, CT_DSPNode *c,
                         CT_DSPNode *d, CT_DSPNodeHandler fn) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_NodeOp4State *state =
        (CT_NodeOp4State *)calloc(1, sizeof(CT_NodeOp4State));
    state->bufA = a->buf;
    state->bufB = b->buf;
    state->bufC = c->buf;
    state->bufD = d->buf;
    node->state = state;
    node->handler = fn;
    return node;
}

CT_DSPNode *ct_synth_copy(char *id, CT_DSPNode *src, CT_DSPNode *dest) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_NodeOp2State *state =
        (CT_NodeOp2State *)calloc(1, sizeof(CT_NodeOp2State));
    state->bufA = src->buf;
    state->bufB = dest->buf;
    node->state = state;
    node->handler = ct_synth_process_copy;
    return node;
}

uint8_t ct_synth_process_mult(CT_DSPNode *node, CT_DSPStack *stack,
                              CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_NodeOp2State *state = (CT_NodeOp2State *)(node->state);
    const float *a = state->bufA;
    float *b = state->bufB;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        *buf++ = (*a++) * (*b++);
    }
    return 0;
}

uint8_t ct_synth_process_sum(CT_DSPNode *node, CT_DSPStack *stack,
                             CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_NodeOp2State *state = (CT_NodeOp2State *)(node->state);
    const float *a = state->bufA;
    float *b = state->bufB;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        *buf++ = (*a++) + (*b++);
    }
    return 0;
}

uint8_t ct_synth_process_copy(CT_DSPNode *node, CT_DSPStack *stack,
                              CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_NodeOp2State *state = (CT_NodeOp2State *)(node->state);
    const float *a = state->bufA;
    float *b = state->bufB;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        *b = *a++;
        *buf++ = *b++;
    }
    return 0;
}

uint8_t ct_synth_process_madd(CT_DSPNode *node, CT_DSPStack *stack,
                              CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_NodeOp4State *state = (CT_NodeOp4State *)(node->state);
    const float *a = state->bufA;
    const float *b = state->bufB;
    const float *c = state->bufC;
    const float *d = state->bufD;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        *buf++ = ((*a++) * (*b++)) + ((*c++) * (*d++));
    }
    return 0;
}

uint8_t ct_synth_process_madd_const(CT_DSPNode *node, CT_DSPStack *stack,
                                    CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_NodeOp4CState *state = (CT_NodeOp4CState *)(node->state);
    const float *a = state->bufA;
    const float *b = state->bufB;
    const float af = state->af;
    const float bf = state->bf;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        *buf++ = ((*a++) * af) + ((*b++) * bf);
    }
    return 0;
}
