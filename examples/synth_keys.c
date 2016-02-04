#include "demo_common.h"

const uint8_t scale[] = {36, 38, 40, 43, 45, 60, 50, 52};
const uint8_t scale2[] = {33, 31, 26, 31, 29, 33, 31, 29};

static void init_voice(CT_Synth *synth, CT_DSPStack *stack, float freq);
static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data);
static void trigger_note(AppState *app, char ch);

int main(int argc, char *argv[]) {
    AppState app;

    srand(time(0));

    ct_synth_init(&app.synth, 8);

    app.synth.lfo[0] = ct_synth_osc("lfo1", ct_synth_process_osc_sin, 0.0f,
                                    HZ_TO_RAD(0.1f), 0.5f, 1.0f);
    app.synth.numLFO = 1;

    for (uint8_t i = 0; i < app.synth.numStacks; i++) {
        init_voice(&app.synth, &app.synth.stacks[i], ct_synth_notes[scale[0]]);
    }

    app.pitch = 28;
    app.callback = render_synth;
    app.handler = trigger_note;
    app.channels = 1;
    app.isNewNote = 0;
    return demo(&app);
}

static void init_voice(CT_Synth *synth, CT_DSPStack *stack, float freq) {
    ct_synth_init_stack(stack);
    CT_DSPNode *env =
        ct_synth_adsr("env", synth->lfo[0], 2.0f, 0.7f, 1.5f, 0.5f, 0.5f);
    CT_DSPNode *lfo =
        ct_synth_osc("lfoPitch", ct_synth_process_osc_square, 0.0f,
                     HZ_TO_RAD(2.0f), HZ_TO_RAD(1.0f), 0.0f);
    CT_DSPNode *lfo2 = ct_synth_osc("lfoGain", ct_synth_process_osc_saw, 0.0f,
                                    HZ_TO_RAD(6.666666f), 0.4f, 0.6f);
    CT_DSPNode *osc1 = ct_synth_osc("osc1", ct_synth_process_osc_saw, 0.0f,
                                    HZ_TO_RAD(freq * 0.51f), 0.35f, 0.0f);
    ct_synth_set_osc_lfo(osc1, lfo, 1.0f);
    CT_DSPNode *osc2 = ct_synth_osc("osc2", ct_synth_process_osc_saw, 0.0f,
                                    HZ_TO_RAD(freq), 0.35f, 0.0f);
    ct_synth_set_osc_lfo(osc2, lfo, 1.0f);
    ct_synth_set_osc_env(osc2, env, HZ_TO_RAD(10.0f));
    CT_DSPNode *sum =
        ct_synth_op4("sum", osc1, env, osc2, env, ct_synth_process_madd);
    CT_DSPNode *gate = ct_synth_op2("gate", sum, lfo2, ct_synth_process_mult);
    // CT_DSPNode *filter = ct_synth_filter_iir("filter", IIR_LP, gate, NULL,
    // ct_randf(300.0f, 1200.0f),
    //                  0.85f);
    CT_DSPNode *filter = ct_synth_filter_formant("filter", rand() % 5, gate);
    CT_DSPNode *delay =
        ct_synth_delay("delay", filter, SAMPLE_RATE >> 2, 0.4f, 1);
    CT_DSPNode *filter2 =
        ct_synth_filter_iir("filter2", IIR_HP, delay, NULL, 1800.0f, 0.0f);
    CT_DSPNode *delay2 = ct_synth_delay(
        "delay2", filter2, (uint32_t)(SAMPLE_RATE * 0.333f), 0.4, 1);
    CT_DSPNode *delay3 = ct_synth_delay(
        "delay3", delay2, (uint32_t)(SAMPLE_RATE * 0.4781f), 0.4f, 1);
    CT_DSPNode *sum2 = ct_synth_op4_const("sum2", filter, 0.2f, delay3, 0.8f,
                                          ct_synth_process_madd_const);
    CT_DSPNode *nodes[] = {env,    lfo,   lfo2,    osc1,   osc2,   sum, gate,
                           filter, delay, filter2, delay2, delay3, sum2};
    ct_synth_build_stack(stack, nodes, 13);
}

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    AppState *app = (AppState *)data;
    if (app->isNewNote) {
        float freq = ct_synth_notes[scale[app->noteID % 8]];
        float freq2 = ct_synth_notes[scale2[(app->noteID % 8)] + 12];
        CT_DSPStack *s = &app->synth.stacks[app->voiceID];
        ct_synth_reset_adsr(NODE_ID(s, "env"));
        NODE_ID_STATE(CT_OscState, s, "osc1")->freq = HZ_TO_RAD(freq2);
        NODE_ID_STATE(CT_OscState, s, "osc2")->freq = HZ_TO_RAD(freq);
        ct_synth_activate_stack(s);
        app->noteID++;
        app->voiceID = (app->voiceID + 1) % app->synth.numStacks;
        app->isNewNote = 0;
    }

    ct_synth_update_mix_mono_f32(&app->synth, frames, (float *)out);
    return 0;
}

static void trigger_note(AppState *app, char ch) {
    app->isNewNote = 1;
}
