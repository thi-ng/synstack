#include "synth.h"

// ( thresh amp -- fb-buf )
CTSS_DECL_OP(init_foldback) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2);
    CTSS_VMValue *dsp = vm->dsp - 1;
    CTSS_SynthOp *op = (CTSS_SynthOp *)calloc(1, sizeof(CTSS_SynthOp));
    CTSS_FoldbackState *fb =
        (CTSS_FoldbackState *)calloc(1, sizeof(CTSS_FoldbackState));
    fb->amp = (*dsp).f32;
    fb->threshold = (*(dsp - 1)).f32;
    op->state = fb;
    CTSS_VMValue v = {.synthOp = op};
    *(dsp - 1) = v;
    vm->dsp = dsp;
}

// ( fb src -- fb )
CTSS_DECL_OP(process_foldback) {
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2);
    CTSS_SynthOp *op = CTSS_SYNTH_OP(vm->dsp - 2);
    float *src = CTSS_BUF(vm->dsp - 1);
    float *buf = op->buf;
    CTSS_FoldbackState *fb = (CTSS_FoldbackState *)(op->state);
    const float thresh = fb->threshold;
    const float thresh2 = thresh * 2.0f;
    const float thresh4 = thresh * 4.0f;
    const float amp = fb->amp;
    uint32_t len = CTSS_AUDIO_BUFFER_SIZE;
    while (len--) {
        float in = *src++;
        if (in > thresh || in < -thresh) {
            in = (fabs(fabs(fmod(in - thresh, thresh4)) - thresh2) - thresh);
        }
        *buf++ = in * amp;
    }
    vm->dsp--;
}

void ctss_init_foldback_ops(CTSS_VM *vm) {
    CTSS_DEFNATIVE(">foldback", init_foldback);
    CTSS_DEFNATIVE("foldback", process_foldback);
}
