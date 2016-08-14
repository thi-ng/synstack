#include "synth.h"

// ( att dec rel again sgain -- env )
CTSS_DECL_OP(init_adsr) {
  CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 5);
  CTSS_VMValue *dsp   = vm->dsp - 5;
  CTSS_SynthOp *op    = (CTSS_SynthOp *)calloc(1, sizeof(CTSS_SynthOp));
  CTSS_ADSRState *env = (CTSS_ADSRState *)calloc(1, sizeof(CTSS_ADSRState));
  env->attackRate     = TIME_TO_FS_RATE((*dsp).f32);
  env->decayRate      = TIME_TO_FS_RATE((*(dsp + 1)).f32);
  env->releaseRate    = TIME_TO_FS_RATE((*(dsp + 2)).f32);
  env->attackGain     = (*(dsp + 3)).f32;
  env->sustainGain    = (*(dsp + 4)).f32;
  env->phase          = ATTACK;
  op->state           = env;
  CTSS_VMValue v = {.synthOp = op};
  *dsp    = v;
  vm->dsp = dsp + 1;
}

// ( env -- )
CTSS_DECL_OP(reset_adsr) {
  CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1);
  CTSS_ADSRState *env = (CTSS_ADSRState *)((CTSS_SYNTH_OP(vm->dsp - 1))->state);
  env->phase          = ATTACK;
  env->currGain       = 0.0f;
  vm->dsp--;
}

// ( env -- )
CTSS_DECL_OP(dump_adsr) {
  CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1);
  CTSS_SynthOp *op    = CTSS_SYNTH_OP(vm->dsp - 1);
  CTSS_ADSRState *env = (CTSS_ADSRState *)(op->state);
  CTSS_PRINT(("adsr: %f %f %f %f %f gain: %f phase: %u buf: %p\n",
              env->attackRate, env->decayRate, env->releaseRate,
              env->attackGain, env->sustainGain, env->currGain, env->phase,
              op->buf));
}

// ( env mod -- env )
CTSS_DECL_OP(process_adsr) {
  CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2);
  CTSS_SynthOp *op     = CTSS_SYNTH_OP(vm->dsp - 2);
  float *mod           = CTSS_BUF(vm->dsp - 1);
  CTSS_ADSRState *env  = (CTSS_ADSRState *)(op->state);
  float *buf           = op->buf;
  float gain           = env->currGain;
  CTSS_ADSRPhase phase = env->phase;
  uint32_t len         = CTSS_AUDIO_BUFFER_SIZE;
  while (len--) {
    switch (phase) {
      case ATTACK:
        gain += env->attackRate * (*mod);
        if (gain >= env->attackGain) {
          gain  = env->attackGain;
          phase = DECAY;
        }
        break;
      case DECAY:
        gain -= env->decayRate * (*mod);
        if (gain <= env->sustainGain) {
          gain  = env->sustainGain;
          phase = RELEASE;
        }
        break;
      case SUSTAIN:
        // TODO
        break;
      case RELEASE:
        gain -= env->releaseRate;
        if (gain <= 0.0f) {
          gain  = 0.0f;
          phase = IDLE;
        }
        break;
      default:
        break;
    }
    *buf++ = gain;
    mod++;
  }
  env->currGain = gain;
  env->phase    = phase;
  vm->dsp--;
}

void ctss_init_adsr_ops(CTSS_VM *vm) {
  CTSS_DEFNATIVE(">adsr", init_adsr);
  CTSS_DEFNATIVE("reset-adsr", reset_adsr);
  CTSS_DEFNATIVE(".adsr", dump_adsr);
  CTSS_DEFNATIVE("adsr", process_adsr);
}
