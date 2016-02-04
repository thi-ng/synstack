#include "adsr.h"

CT_DSPNode *ct_synth_adsr(char *id, CT_DSPNode *lfo, float attTime,
                          float decayTime, float releaseTime, float attGain,
                          float sustainGain) {
    sustainGain = (sustainGain > 0.99f ? 0.99f : sustainGain);
    CT_DSPNode *node = ct_synth_node(id, 1);
    CT_ADSRState *env = (CT_ADSRState *)calloc(1, sizeof(CT_ADSRState));
    env->lfo = (lfo != NULL ? lfo->buf : ct_synth_zero);
    node->state = env;
    node->handler = ct_synth_process_adsr;
    ct_synth_configure_adsr(node, attTime, decayTime, releaseTime, attGain,
                            sustainGain);
    ct_synth_reset_adsr(node);
    return node;
}

void ct_synth_configure_adsr(CT_DSPNode *node, float attTime, float decayTime,
                             float releaseTime, float attGain,
                             float sustainGain) {
    CT_ADSRState *env = (CT_ADSRState *)(node->state);
    env->attackRate = TIME_TO_FS_RATE(attTime) * attGain;
    env->decayRate = TIME_TO_FS_RATE(decayTime) * (attGain - sustainGain);
    env->releaseRate = TIME_TO_FS_RATE(releaseTime) * sustainGain;
    env->attackGain = attGain;
    env->sustainGain = sustainGain;
}

void ct_synth_reset_adsr(CT_DSPNode *node) {
    CT_ADSRState *state = (CT_ADSRState *)(node->state);
    state->phase = ATTACK;
    state->currGain = 0.0f;
}

uint8_t ct_synth_process_adsr(CT_DSPNode *node, CT_DSPStack *stack,
                              CT_Synth *synth, uint32_t offset) {
    CT_UNUSED(synth);
    CT_UNUSED(stack);
    CT_ADSRState *state = (CT_ADSRState *)(node->state);
    float *buf = node->buf + offset;
    float *envMod = state->lfo;
    // CT_ADSRPhase prevPhase = state->phase;
    uint32_t len = AUDIO_BUFFER_SIZE - offset;
    if (envMod != NULL) {
        envMod += offset;
        while (len--) {
            switch (state->phase) {
            case ATTACK:
                state->currGain += state->attackRate * (*envMod);
                if (state->currGain >= state->attackGain) {
                    state->currGain = state->attackGain;
                    state->phase = DECAY;
                }
                break;
            case DECAY:
                state->currGain -= state->decayRate * (*envMod);
                if (state->currGain <= state->sustainGain) {
                    state->currGain = state->sustainGain;
                    state->phase = RELEASE;
                }
                break;
            case SUSTAIN:
                // TODO
                break;
            case RELEASE:
                state->currGain -= state->releaseRate;
                if (state->currGain <= 0.0f) {
                    state->currGain = 0.0f;
                    state->phase = IDLE;
                }
                break;
            default:
                break;
            }
            *buf++ = state->currGain;
            envMod++;
        }
    } else {
        while (len--) {
            switch (state->phase) {
            case ATTACK:
                state->currGain += state->attackRate;
                if (state->currGain >= state->attackGain) {
                    state->currGain = state->attackGain;
                    state->phase = DECAY;
                }
                break;
            case DECAY:
                state->currGain -= state->decayRate;
                if (state->currGain <= state->sustainGain) {
                    state->currGain = state->sustainGain;
                    state->phase = RELEASE;
                }
                break;
            case SUSTAIN:
                // TODO
                break;
            case RELEASE:
                state->currGain -= state->releaseRate;
                if (state->currGain <= 0.0f) {
                    state->currGain = 0.0f;
                    state->phase = IDLE;
                }
                break;
            default:
                break;
            }
            *buf++ = state->currGain;
        }
    }
    return 0; //((prevPhase == IDLE) && (state->phase == IDLE)) ? STACK_ACTIVE :
              // 0;
}
