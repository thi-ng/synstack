#include "adsr.h"

CTSS_DSPNode *ctss_adsr(char *id,
                        CTSS_DSPNode *lfo,
                        float attTime,
                        float decayTime,
                        float releaseTime,
                        float attGain,
                        float sustainGain) {
  sustainGain         = CT_MIN(sustainGain, 0.99f);
  CTSS_DSPNode *node  = ctss_node(id, 1);
  CTSS_ADSRState *env = calloc(1, sizeof(CTSS_ADSRState));
  env->lfo            = (float *)(lfo != NULL ? lfo->buf : ctss_zero);
  node->state         = env;
  node->handler       = ctss_process_adsr;
  ctss_configure_adsr(node, attTime, decayTime, releaseTime, attGain,
                      sustainGain);
  ctss_reset_adsr(node);
  return node;
}

void ctss_configure_adsr(CTSS_DSPNode *node,
                         float attTime,
                         float decayTime,
                         float releaseTime,
                         float attGain,
                         float sustainGain) {
  CTSS_ADSRState *env = node->state;
  env->attackRate     = TIME_TO_FS_RATE(attTime) * attGain;
  env->decayRate      = TIME_TO_FS_RATE(decayTime) * (attGain - sustainGain);
  env->releaseRate    = TIME_TO_FS_RATE(releaseTime) * sustainGain;
  env->attackGain     = attGain;
  env->sustainGain    = sustainGain;
}

void ctss_reset_adsr(CTSS_DSPNode *node) {
  CTSS_ADSRState *state = node->state;
  state->phase          = ATTACK;
  state->currGain       = 0.0f;
}

uint8_t ctss_process_adsr(CTSS_DSPNode *node,
                          CTSS_DSPStack *stack,
                          CTSS_Synth *synth) {
  CTSS_UNUSED(synth);
  CTSS_UNUSED(stack);
  CTSS_ADSRState *state = node->state;
  float *buf            = node->buf;
  float *envMod         = state->lfo;
  // CTSS_ADSRPhase prevPhase = state->phase;
  uint32_t len = AUDIO_BUFFER_SIZE;
  if (envMod != NULL) {
    while (len--) {
      switch (state->phase) {
        case ATTACK:
          state->currGain += state->attackRate * (*envMod);
          if (state->currGain >= state->attackGain) {
            state->currGain = state->attackGain;
            state->phase    = DECAY;
          }
          break;
        case DECAY:
          state->currGain -= state->decayRate * (*envMod);
          if (state->currGain <= state->sustainGain) {
            state->currGain = state->sustainGain;
            state->phase    = RELEASE;
          }
          break;
        case SUSTAIN:
          // TODO
          break;
        case RELEASE:
          state->currGain -= state->releaseRate;
          if (state->currGain <= 0.0f) {
            state->currGain = 0.0f;
            state->phase    = IDLE;
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
            state->phase    = DECAY;
          }
          break;
        case DECAY:
          state->currGain -= state->decayRate;
          if (state->currGain <= state->sustainGain) {
            state->currGain = state->sustainGain;
            state->phase    = RELEASE;
          }
          break;
        case SUSTAIN:
          // TODO
          break;
        case RELEASE:
          state->currGain -= state->releaseRate;
          if (state->currGain <= 0.0f) {
            state->currGain = 0.0f;
            state->phase    = IDLE;
          }
          break;
        default:
          break;
      }
      *buf++ = state->currGain;
    }
  }
  return 0;  //((prevPhase == IDLE) && (state->phase == IDLE)) ? STACK_ACTIVE :
             // 0;
}
