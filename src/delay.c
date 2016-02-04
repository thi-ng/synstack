#include "delay.h"

CT_DSPNode *ct_synth_delay(char *id, CT_DSPNode *src, uint32_t len,
                           float feedback, uint8_t channels) {
    CT_DSPNode *node = ct_synth_node(id, channels);
    CT_DelayState *state = (CT_DelayState *)calloc(1, sizeof(CT_DelayState));
    state->delayLine = (float *)calloc(len * channels, sizeof(float));
    state->src = src->buf;
    state->len = len * channels;
    state->feedback = feedback;
    state->channels = channels;
    state->writePos = 0;
    state->readPos = channels;
    state->writePtr = state->delayLine;
    state->readPtr = state->delayLine + channels;
    node->state = state;
    node->handler = ct_synth_process_delay;
    return node;
}

uint8_t ct_synth_process_delay(CT_DSPNode *node, CT_DSPStack *stack,
                               CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_DelayState *state = (CT_DelayState *)node->state;
    float *read = state->readPtr;
    float *write = state->writePtr;
    float *src = state->src + offset * state->channels;
    float *buf = node->buf + offset * state->channels;
    uint32_t len = (AUDIO_BUFFER_SIZE - offset) * state->channels;
    while (len--) {
        *write = (*src++) + (*read++) * state->feedback;
        *buf++ = *write++;
        if (++state->readPos == state->len) {
            state->readPos = 0;
            read = state->delayLine;
        }
        if (++state->writePos == state->len) {
            state->writePos = 0;
            write = state->delayLine;
        }
    }
    state->readPtr = read;
    state->writePtr = write;
    return 0;
}
