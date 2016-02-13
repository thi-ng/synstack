#include "synth.h"

CTSS_DECL_OP(hz) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1);
    (*(vm->dsp - 1)).f32 = CTSS_HZ_TO_RAD((*(vm->dsp - 1)).f32);
}

CTSS_DECL_OP(dump_osc) {
    CTSS_SynthOp *op = CTSS_SYNTH_OP(vm->dsp - 1);
    CTSS_PRINT(("phase: %f buf: %p\n", ((CTSS_OscState *)(op->state))->phase,
                op->buf));
}

// ( phase -- osc-buf )
CTSS_DECL_OP(init_osc) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1);
    CTSS_VMValue *dsp = vm->dsp - 1;
    CTSS_SynthOp *op = (CTSS_SynthOp *)calloc(1, sizeof(CTSS_SynthOp));
    CTSS_OscState *osc = (CTSS_OscState *)calloc(1, sizeof(CTSS_OscState));
    osc->phase = (*dsp).f32;
    op->state = osc;
    CTSS_VMValue v = {.synthOp = op};
    *dsp = v;
}

// ( osc-buf freq-buf -- osc-buf )
CTSS_DECL_OP(process_osc_sin) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2);
    CTSS_SynthOp *op = CTSS_SYNTH_OP(vm->dsp - 2);
    float *freq = CTSS_BUF(vm->dsp - 1);
    float *buf = op->buf;
    CTSS_OscState *osc = (CTSS_OscState *)(op->state);
    float phase = osc->phase;
    uint32_t len = CTSS_AUDIO_BUFFER_SIZE;
    while (len--) {
        phase += *freq++;
        TRUNC_PHASE(phase);
        *buf++ = sinf(phase);
    }
    osc->phase = phase;
    vm->dsp--;
}

CTSS_DECL_OP(process_osc_square) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2);
    CTSS_SynthOp *op = CTSS_SYNTH_OP(vm->dsp - 2);
    float *freq = CTSS_BUF(vm->dsp - 1);
    float *buf = op->buf;
    CTSS_OscState *osc = (CTSS_OscState *)(op->state);
    float phase = osc->phase;
    uint32_t len = CTSS_AUDIO_BUFFER_SIZE;
    while (len--) {
        phase += *freq++;
        TRUNC_PHASE(phase);
        *buf++ = phase < PI ? -1.0f : 1.0f;
    }
    osc->phase = phase;
    vm->dsp--;
}

CTSS_DECL_OP(process_osc_saw) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2);
    CTSS_SynthOp *op = CTSS_SYNTH_OP(vm->dsp - 2);
    float *freq = CTSS_BUF(vm->dsp - 1);
    float *buf = op->buf;
    CTSS_OscState *osc = (CTSS_OscState *)(op->state);
    float phase = osc->phase;
    uint32_t len = CTSS_AUDIO_BUFFER_SIZE;
    while (len--) {
        phase += *freq++;
        TRUNC_PHASE(phase);
        *buf++ = -phase * INV_PI + 1.0f;
    }
    osc->phase = phase;
    vm->dsp--;
}

void ctss_init_osc_ops(CTSS_VM *vm) {
    CTSS_DEFNATIVE("hz", hz);
    CTSS_DEFNATIVE(".osc", dump_osc);
    CTSS_DEFNATIVE(">osc", init_osc);
    CTSS_DEFNATIVE("osc-sin", process_osc_sin);
    CTSS_DEFNATIVE("osc-square", process_osc_square);
    CTSS_DEFNATIVE("osc-saw", process_osc_saw);
}
