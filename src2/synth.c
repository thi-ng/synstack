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
    ctss_vm_interpret(&synth.vm,
                      //": setup "
                      "  110.0f hz buf dup val> pitch b! drop "
                      "  1.0f buf dup val> envmod b! drop "
                      "  0.005f 0.01f 0.1f 1.0f 0.25f >adsr val> env "
                      "  0.0f >osc val> osc1 "
                      ": update "
                      "  env envmod adsr "
                      "  osc1 pitch osc-sin "
                      "  swap b*! .b ; "
                      ": render begin update 1 - dup 0 == until drop ; "
                      );
    repl(&synth.vm);
    return 0;
}
