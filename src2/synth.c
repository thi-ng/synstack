#include "core_dict.h"
#include "synth.h"

void ctss_init(CTSS_Synth *synth) {
    CTSS_VM *vm = &synth->vm;
    ctss_vm_init(vm);
    ctss_vm_init_primitives(vm);

    ctss_init_buffer_ops(vm);
    ctss_init_osc_ops(vm);
    ctss_init_adsr_ops(vm);

    ctss_vm_interpret(vm, ctss_vm_core_dict);
}

void repl(CTSS_VM *vm) {
    char buf[256];
    while (fgets(buf, 255, stdin)) {
        ctss_vm_interpret(vm, buf);
    }
}

int main() {
    CTSS_Synth synth;
    ctss_init(&synth);
    repl(&synth.vm);
    return 0;
}
