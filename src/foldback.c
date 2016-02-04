#include <math.h>
#include "foldback.h"

CT_DSPNode *ct_synth_foldback(char *id, CT_DSPNode *src, float threshold,
                              float amp) {
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_FoldbackState *state =
        (CT_FoldbackState *)calloc(1, sizeof(CT_FoldbackState));
    state->src = src->buf;
    state->threshold = threshold;
    state->amp = amp;
    node->state = state;
    node->handler = ct_synth_process_foldback;
    return node;
}

uint8_t ct_synth_process_foldback(CT_DSPNode *node, CT_DSPStack *stack,
                                  CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_FoldbackState *state = (CT_FoldbackState *)(node->state);
    const float *src = state->src;
    const float thresh = state->threshold;
    const float thresh2 = thresh * 2.0f;
    const float thresh4 = thresh * 4.0f;
    const float amp = state->amp;
    float *buf = node->buf;
    uint32_t len = AUDIO_BUFFER_SIZE;
    while (len--) {
        float in = *src++;
        if (in > thresh || in < -thresh) {
            in = (fabs(fabs(fmod(in - thresh, thresh4)) - thresh2) - thresh);
        }
        *buf++ = in * amp;
    }
    return 0;
}
