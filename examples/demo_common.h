#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <ncurses.h>

#include <portaudio.h>

#include "synth.h"
#include "adsr.h"
#include "biquad.h"
#include "delay.h"
#include "foldback.h"
#include "formant.h"
#include "iir.h"
#include "node_ops.h"
#include "osc.h"
#include "panning.h"
#include "pluck.h"

typedef PaStreamCallback DemoRenderFn;
typedef struct AppState AppState;

typedef void (*DemoKeyHandler)(AppState *app, char ch);

struct AppState {
    CT_Synth synth;
    DemoRenderFn *callback;
    DemoKeyHandler handler;
    uint8_t isNewNote;
    uint8_t noteID;
    uint8_t voiceID;
    uint8_t channels;
    int8_t pitch;
    float time;
};

void init_voice_buffers(AppState *app);

uint8_t demo(AppState *app);
void demo_key_handler(AppState *app, char ch);
