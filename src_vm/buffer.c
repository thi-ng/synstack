#include "synth.h"

#define CTSS_DECL_BUF_OP(name, op)              \
  CTSS_DECL_OP(name) {                          \
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)     \
    CTSS_VMValue *dsp = vm->dsp - 1;            \
    float *a          = CTSS_BUF(dsp);          \
    float *b          = CTSS_BUF(dsp - 1);      \
    float *aa         = a;                      \
    float *bb         = b;                      \
    uint32_t len      = CTSS_AUDIO_BUFFER_SIZE; \
    while (len--) {                             \
      *bb = (*aa++)op(*bb);                     \
      *bb++;                                    \
    }                                           \
    vm->dsp = dsp;                              \
  }

CTSS_DECL_BUF_OP(add_buf, +)
CTSS_DECL_BUF_OP(mul_buf, *)

// ( -- buf )
CTSS_DECL_OP(make_buf) {
  CTSS_VM_BOUNDS_CHECK_HI(dsp, ds, DS, 1);
  CTSS_VMValue v = {.synthOp = (CTSS_SynthOp *)calloc(1, sizeof(CTSS_SynthOp))};
  *vm->dsp++ = v;
}

// ( freq buf -- buf )
CTSS_DECL_OP(fill_buf) {
  CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2);
  CTSS_VMValue *dsp = vm->dsp - 1;
  float *addr       = CTSS_BUF(dsp);
  const float x     = (*(dsp - 1)).f32;
  uint32_t len      = CTSS_AUDIO_BUFFER_SIZE;
  while (len--) {
    *addr++ = x;
  }
  *(dsp - 1) = *dsp;
  vm->dsp    = dsp;
}

// ( buf -- )
CTSS_DECL_OP(dump_buf) {
  CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1);
  float *addr  = CTSS_BUF(--vm->dsp);
  uint32_t len = CTSS_AUDIO_BUFFER_SIZE;
  while (len--) {
    CTSS_PRINT(("%f ", *addr++));
  }
  CTSS_PRINT(("\n"));
}

void ctss_init_buffer_ops(CTSS_VM *vm) {
  CTSS_DEFNATIVE("buf", make_buf);
  CTSS_DEFNATIVE("b+!", add_buf);
  CTSS_DEFNATIVE("b*!", mul_buf);
  CTSS_DEFNATIVE("b!", fill_buf);
  CTSS_DEFNATIVE(".b", dump_buf);
}
