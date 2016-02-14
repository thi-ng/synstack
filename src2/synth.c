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

uint32_t noteID = 0;
double lastNote = 0;
const uint8_t scale[] = {36, 40, 43, 45, 55, 52, 48, 60};
const uint8_t scale2[] = {33, 31, 26, 31, 29, 33, 31, 29};

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    CTSS_VM *vm = &synth.vm;
    uint32_t update = ctss_vm_find_word(vm, "update", vm->latest);
    if (timeInfo->currentTime - lastNote >= 0.15) {
        CTSS_VMValue v = {.f32 = ctss_notes[scale[rand() & 7] - 12]};
        *(vm->dsp++) = v;
        ctss_vm_interpret(vm, "new-note");
        lastNote = timeInfo->currentTime;
        noteID = (noteID + 1) % 8;
    }
    float *res = (float *)out;
    for (uint32_t i = 0; i < frames / CTSS_AUDIO_BUFFER_SIZE; i++) {
        ctss_vm_execute_word(vm, update);
        CTSS_VMValue *dsp = vm->dsp - 1;
        float *buf = CTSS_BUF(dsp);
        memcpy(res, buf, CTSS_AUDIO_BUFFER_SIZE * sizeof(float));
        vm->dsp = dsp;
        res += CTSS_AUDIO_BUFFER_SIZE;
    }
    return 0;
}
