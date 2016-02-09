#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "core_dict.h"

#define CTSS_AUDIO_BUFFER_SIZE 32

#define CTSS_DECL_BUF_OP(name, op)              \
  CTSS_DECL_OP(name) {                          \
    CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2)     \
      CTSS_VMValue *dsp = vm->dsp - 1;          \
    float *a = (*dsp).buf;                      \
    float *b = (*(dsp - 1)).buf;                \
    float *aa = a;                              \
    float *bb = b;                              \
    uint32_t len = CTSS_AUDIO_BUFFER_SIZE;      \
    while (--len) {                             \
      *bb = (*aa++) op (*bb);                   \
      *bb++;                                    \
    }                                           \
    (*(dsp - 1)).buf = b;                       \
    vm->dsp = dsp;                              \
  }

CTSS_DECL_BUF_OP(add_buf, +)
CTSS_DECL_BUF_OP(mul_buf, *)

CTSS_DECL_OP(make_buf) {
  CTSS_VM_BOUNDS_CHECK_HI(dsp, ds, DS, 1);
  float *buf = (float*)calloc(CTSS_AUDIO_BUFFER_SIZE, sizeof(float));
  CTSS_VMValue v = {.buf = buf };
  *vm->dsp++ = v;
}

CTSS_DECL_OP(fill_buf) {
  CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 2);
  CTSS_VMValue *dsp = vm->dsp - 1;
  float *addr = (*dsp).buf;
  const float x = (*(dsp - 1)).f32;
  uint32_t len = CTSS_AUDIO_BUFFER_SIZE;
  while (--len) {
    *addr++ = x;
  }
  *(dsp - 1) = *dsp;
  vm->dsp = dsp;
}

CTSS_DECL_OP(dump_buf) {
  CTSS_VM_BOUNDS_CHECK_LO(dsp, ds, DS, 1);
  float *addr = (*(--vm->dsp)).buf;
  uint32_t len = CTSS_AUDIO_BUFFER_SIZE;
  while (--len) {
    CTSS_PRINT(("%f ", *addr++));
  }
  CTSS_PRINT(("\n"));
}

void repl(CTSS_VM *vm) {
  char buf[256];
  while (fgets(buf, 255, stdin)) {
    ctss_vm_interpret(vm, buf);
  }
}

void init_buffer_ops(CTSS_VM *vm) {
  CTSS_DEFNATIVE("mk-buffer", make_buf);
  CTSS_DEFNATIVE("b+!", add_buf);
  CTSS_DEFNATIVE("b*!", mul_buf);
  CTSS_DEFNATIVE("b!", fill_buf);
  CTSS_DEFNATIVE(".b", dump_buf);
}

int main() {
  printf("CTSS_VM:\t%lu\n", sizeof(CTSS_VM));

  CTSS_VM vm;
  ctss_vm_init(&vm);
  // ctss_vm_dump(&vm);

  ctss_vm_init_primitives(&vm);
  init_buffer_ops(&vm);

  ctss_vm_interpret(&vm, ctss_vm_core_dict);
  repl(&vm);

  return 0;
}
