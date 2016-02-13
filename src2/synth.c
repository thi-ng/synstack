#include <string.h>
#include <portaudio.h>
#include <math.h>
#include "core_dict.h"
#include "synth.h"

CTSS_Synth synth;

extern const float ctss_notes[96];

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data);

void ctss_init(CTSS_Synth *synth) {
    CTSS_VM *vm = &synth->vm;
    ctss_vm_init(vm);
    ctss_vm_init_primitives(vm);
    ctss_vm_interpret(vm, ctss_vm_core_dict);

    ctss_init_buffer_ops(vm);
    ctss_init_osc_ops(vm);
    ctss_init_adsr_ops(vm);
    ctss_init_foldback_ops(vm);
}

void repl(CTSS_VM *vm) {
    char buf[256];
    while (fgets(buf, 255, stdin)) {
        ctss_vm_interpret(vm, buf);
    }
}

int main() {
    ctss_init(&synth);

    ctss_vm_interpret(&synth.vm,
                      "buf var> pitch ! "
                      "buf var> pitch2 ! "
                      "0.125f hz buf dup var> lfo1pitch ! b! drop "
                      "0.9f buf dup var> lfo1amp ! b! drop "
                      "1.0f buf dup var> lfo1dc ! b! drop "
                      "4.0f hz buf dup var> lfo2pitch ! b! drop "
                      "0.0f hz buf dup var> lfo2amp ! b! drop "

                      "0.0f >osc var> lfo1 ! "
                      "0.0f >osc var> lfo2 ! "
                      "0.0f >osc var> osc1 ! "
                      "0.0f >osc var> osc2 ! "
                      "0.005f 0.1f 1.0f 1.0f 0.25f >adsr var> env ! "
                      "0.025f 10.0f >foldback var> fb ! "

                      ": set-pitch ( freq -- ) "
                      "  hz dup dup pitch @ b! drop "
                      "  1.01f f* pitch2 @ b! drop "
                      "  0.0f f* lfo2amp @ b! drop ; "

                      ": update "
                      "  lfo1 @ lfo1pitch @ osc-sin lfo1amp @ b*! lfo1dc @ b+! "
                      "  env @ swap adsr dup "
                      "  lfo2 @ lfo2pitch @ osc-sin lfo2amp @ b*! pitch @ b+! "
                      "  osc1 @ swap osc-sin "
                      "  swap b*! swap "
                      "  osc2 @ pitch2 @ osc-sin "
                      "  swap b*! b+! fb @ swap foldback ; "); // */

    // repl(&synth.vm);

    PaStream *stream;
    Pa_Initialize();
    PaError err = Pa_OpenDefaultStream(&stream, 0, /* no input channels */
                                       1, paFloat32, CTSS_SAMPLE_RATE,
                                       256, /* frames per buffer */
                                       render_synth, NULL);
    if (err == paNoError) {
        err = Pa_StartStream(stream);
        if (err == paNoError) {
            // repl(&synth.vm);
            fgetc(stdin);
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
        }
    }
    Pa_Terminate();
    return 0;
}

uint32_t noteID = 0;
double lastNote = 0;

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    CTSS_VM *vm = &synth.vm;
    uint32_t update = ctss_vm_find_word(vm, "update", vm->latest);
    if (timeInfo->currentTime - lastNote >= 0.15) {
        CTSS_VMValue v = {.f32 = ctss_notes[24 + noteID]};
        *(vm->dsp++) = v;
        ctss_vm_interpret(vm,
                          "set-pitch 0.005f 0.1f 0.5f 0.6f 0.25f >adsr env !");
        lastNote = timeInfo->currentTime;
        noteID = (noteID + 2) % 24;
    }
    float *res = (float *)out;
    for (uint32_t i = 0; i < frames / CTSS_AUDIO_BUFFER_SIZE; i++) {
        ctss_vm_execute_word(vm, update);
        CTSS_VMValue *dsp = vm->dsp - 1;
        float *buf = CTSS_BUF(dsp);
        memcpy(res, buf, CTSS_AUDIO_BUFFER_SIZE * 4);
        vm->dsp = dsp;
        res += CTSS_AUDIO_BUFFER_SIZE;
    }
    return 0;
}
