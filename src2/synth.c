#include <string.h>
#include <portaudio.h>
#include <math.h>
#include "core_dict.h"
#include "synth_dict.h"
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

    ctss_vm_interpret(vm, ctss_synth_dict);
}

void repl(CTSS_VM *vm) {
    char buf[256];
    while (fgets(buf, 255, stdin)) {
        ctss_vm_interpret(vm, buf);
    }
}

int main(int argc, char **argv) {
    ctss_init(&synth);
    if (argc == 2) {
        ctss_vm_interpret_file(&synth.vm, argv[1], 4096);
        PaStream *stream;
        Pa_Initialize();
        PaError err =
            Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, CTSS_SAMPLE_RATE,
                                 256, render_synth, NULL);
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
    } else {
        repl(&synth.vm);
    }
    return 0;
}

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    CTSS_VM *vm = &synth.vm;
    CTSS_VMValue v = {.f32 = (float)timeInfo->currentTime};
    float *res = (float *)out;
    uint32_t update = ctss_vm_find_word(vm, "update", vm->latest);
    for (uint32_t i = 0; i < frames / CTSS_AUDIO_BUFFER_SIZE; i++) {
        *(vm->dsp++) = v;
        ctss_vm_execute_word(vm, update);
        CTSS_VMValue *dsp = vm->dsp - 1;
        float *buf = CTSS_BUF(dsp);
        memcpy(res, buf, CTSS_AUDIO_BUFFER_SIZE * sizeof(float));
        vm->dsp = dsp;
        res += CTSS_AUDIO_BUFFER_SIZE;
    }
    return 0;
}
