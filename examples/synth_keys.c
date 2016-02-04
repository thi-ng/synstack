#include "demo_common.h"

const uint8_t scale[] = {36, 38, 40, 43, 45, 60, 50, 52};
const uint8_t scale2[] = {33, 31, 26, 31, 29, 33, 31, 29};

static void init_voice(CTSS_Synth *synth, CTSS_DSPStack *stack, float freq);
static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data);
static void trigger_note(AppState *app, char ch);

int main(int argc, char *argv[]) {
    AppState app;

    srand(time(0));

    ctss_init(&app.synth, 8);

    app.synth.lfo[0] = ctss_osc("lfo1", ctss_process_osc_sin, 0.0f,
                                HZ_TO_RAD(0.1f), 0.5f, 1.0f);
    app.synth.numLFO = 1;

    for (uint8_t i = 0; i < app.synth.numStacks; i++) {
        init_voice(&app.synth, &app.synth.stacks[i], ctss_notes[scale[0]]);
    }

    app.pitch = 28;
    app.callback = render_synth;
    app.handler = trigger_note;
    app.channels = 1;
    app.isNewNote = 0;
    return demo(&app);
}

static void init_voice(CTSS_Synth *synth, CTSS_DSPStack *stack, float freq) {
    ctss_init_stack(stack);
    CTSS_DSPNode *env =
        ctss_adsr("env", synth->lfo[0], 2.0f, 0.7f, 1.5f, 0.5f, 0.5f);
    CTSS_DSPNode *lfo = ctss_osc("lfoPitch", ctss_process_osc_square, 0.0f,
                                 HZ_TO_RAD(2.0f), HZ_TO_RAD(1.0f), 0.0f);
    CTSS_DSPNode *lfo2 = ctss_osc("lfoGain", ctss_process_osc_saw, 0.0f,
                                  HZ_TO_RAD(6.666666f), 0.4f, 0.6f);
    CTSS_DSPNode *osc1 = ctss_osc("osc1", ctss_process_osc_saw, 0.0f,
                                  HZ_TO_RAD(freq * 0.51f), 0.35f, 0.0f);
    ctss_set_osc_lfo(osc1, lfo, 1.0f);
    CTSS_DSPNode *osc2 = ctss_osc("osc2", ctss_process_osc_saw, 0.0f,
                                  HZ_TO_RAD(freq), 0.35f, 0.0f);
    ctss_set_osc_lfo(osc2, lfo, 1.0f);
    ctss_set_osc_env(osc2, env, HZ_TO_RAD(10.0f));
    CTSS_DSPNode *sum =
        ctss_op4("sum", osc1, env, osc2, env, ctss_process_madd);
    CTSS_DSPNode *gate = ctss_op2("gate", sum, lfo2, ctss_process_mult);
    // CTSS_DSPNode *filter = ctss_filter_iir("filter", IIR_LP, gate, NULL,
    // ctss_randf(300.0f, 1200.0f),
    //                  0.85f);
    CTSS_DSPNode *filter = ctss_filter_formant("filter", rand() % 5, gate);
    CTSS_DSPNode *delay =
        ctss_delay("delay", filter, SAMPLE_RATE >> 2, 0.4f, 1);
    CTSS_DSPNode *filter2 =
        ctss_filter_iir("filter2", IIR_HP, delay, NULL, 1800.0f, 0.0f);
    CTSS_DSPNode *delay2 =
        ctss_delay("delay2", filter2, (uint32_t)(SAMPLE_RATE * 0.333f), 0.4, 1);
    CTSS_DSPNode *delay3 = ctss_delay(
        "delay3", delay2, (uint32_t)(SAMPLE_RATE * 0.4781f), 0.4f, 1);
    CTSS_DSPNode *sum2 = ctss_op4_const("sum2", filter, 0.2f, delay3, 0.8f,
                                        ctss_process_madd_const);
    CTSS_DSPNode *nodes[] = {env,    lfo,   lfo2,    osc1,   osc2,   sum, gate,
                             filter, delay, filter2, delay2, delay3, sum2};
    ctss_build_stack(stack, nodes, 13);
}

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    AppState *app = (AppState *)data;
    if (app->isNewNote) {
        float freq = ctss_notes[scale[app->noteID % 8]];
        float freq2 = ctss_notes[scale2[(app->noteID % 8)] + 12];
        CTSS_DSPStack *s = &app->synth.stacks[app->voiceID];
        ctss_reset_adsr(NODE_ID(s, "env"));
        NODE_ID_STATE(CTSS_OscState, s, "osc1")->freq = HZ_TO_RAD(freq2);
        NODE_ID_STATE(CTSS_OscState, s, "osc2")->freq = HZ_TO_RAD(freq);
        ctss_activate_stack(s);
        app->noteID++;
        app->voiceID = (app->voiceID + 1) % app->synth.numStacks;
        app->isNewNote = 0;
    }

    ctss_update_mix_mono_f32(&app->synth, ctss_mixdown_f32, frames,
                             (float *)out);
    return 0;
}

static void trigger_note(AppState *app, char ch) {
    app->isNewNote = 1;
}
