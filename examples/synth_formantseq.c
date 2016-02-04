#include "demo_common.h"

const uint8_t scale[] = {36, 40, 43, 45, 55, 52, 48, 60};
const uint8_t scale2[] = {33, 31, 26, 31, 29, 33, 31, 29};
const float pitch[] = {0.25f, 0.5f, 1.0f};
static float formant[] = {130, 1090, 2440, 4400, 1.0, 2.0, 0.3, 0.8};

static void init_voice(CT_Synth *synth, CT_DSPStack *stack, float freq);
static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data);

int main(int argc, char *argv[]) {
    AppState app;

    srand(time(0));

    ct_synth_preinit_osc_formant();
    ct_synth_init(&app.synth, 8);

    app.synth.lfo[0] =
        ct_synth_osc("lfo1", ct_synth_process_osc_sin, PI * 1.25f,
                     HZ_TO_RAD(1 / 24.0f), 0.6f, 0.9f);
    app.synth.numLFO = 1;

    for (uint8_t i = 0; i < app.synth.numStacks; i++) {
        init_voice(&app.synth, &app.synth.stacks[i], ct_synth_notes[scale[0]]);
    }

    app.pitch = -8;
    app.callback = render_synth;
    app.handler = demo_key_handler;
    app.channels = 1;
    return demo(&app);
}

static void init_voice(CT_Synth *synth, CT_DSPStack *stack, float freq) {
    ct_synth_init_stack(stack);
    CT_DSPNode *env =
        ct_synth_adsr("env", synth->lfo[0], 0.005f, 0.05f, 0.02f, 1.0f, 0.99f);
    CT_DSPNode *osc1 =
        ct_synth_osc_formant_id("osc1", 0, freq, 0.6f, 0.0f, 0.0005f);
    // ct_synth_osc_formant("osc1", formant, freq, 0.3f, 0.0f, 0.005f);
    // ct_synth_osc("osc1", ct_synth_process_osc_sawsin, 0.0f, freq, 0.3f,
    // 0.0f);
    CT_DSPNode *imp =
        ct_synth_osc("osc2imp", ct_synth_process_osc_impulse, 0.0f,
                     HZ_TO_RAD(30.0f), HZ_TO_RAD(110.0f), 0.0f);
    CT_DSPNode *osc2 = ct_synth_osc("osc2", ct_synth_process_osc_sawsin, 0.0f,
                                    freq, 0.3f, 0.0f);
    ct_synth_set_osc_lfo(osc2, imp, 1.0f);
    CT_DSPNode *sum = ct_synth_op2("sum", osc1, env, ct_synth_process_mult);
    CT_DSPNode *sum2 = ct_synth_op2("sum2", osc2, env, ct_synth_process_mult);
    CT_DSPNode *fb = ct_synth_foldback("fb", sum, 0.15f, 6.0f);
    CT_DSPNode *filter =
        ct_synth_filter_biquad("filter", LPF, fb, 100.0f, 3.0f, 0.5f);
    CT_DSPNode *filter2 =
        ct_synth_filter_biquad("filter2", HPF, sum, 100.0f, 3.0f, 0.5f);
    CT_DSPNode *delay = ct_synth_delay(
        "delay", filter2, (uint32_t)(SAMPLE_RATE * 3 / 8), 0.2f, 1);
    CT_DSPNode *sum3 = ct_synth_op4_const("sum3", filter, 0.4f, delay, 0.7f,
                                          ct_synth_process_madd_const);
    CT_DSPNode *sum4 = ct_synth_op2("sum4", sum2, sum3, ct_synth_process_sum);
    CT_DSPNode *nodes[] = {env, imp,    osc1,    osc2,  sum,  sum2,
                           fb,  filter, filter2, delay, sum3, sum4};
    ct_synth_build_stack(stack, nodes, 12);
}

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    AppState *app = (AppState *)data;
    float time = (float)timeInfo->currentTime;
    if (time - app->time >= 0.1249f) {
        if ((rand() % 100) < 80) {
            float freq = ct_synth_notes[scale[app->noteID % 8] + app->pitch];
            float freq2 = ct_synth_notes[(scale[app->noteID % 8] % 12) + 12 +
                                         (app->pitch % 12)];
            CT_DSPStack *s = &app->synth.stacks[app->voiceID];
            ct_synth_reset_adsr(NODE_ID(s, "env"));
            CT_FormantOsc *osc = NODE_ID_STATE(CT_FormantOsc, s, "osc1");
            CT_OscState *osc2 = NODE_ID_STATE(CT_OscState, s, "osc2");
            osc->freq = HZ_TO_RAD(freq * pitch[rand() % 3]);
            osc2->freq = HZ_TO_RAD(freq2);
            NODE_ID_STATE(CT_OscState, s, "osc2imp")->phase = 0;
            // for (int i = 0; i < 4; i++) {
            //    formant[i] = ct_randf(110, 4400);
            //    formant[i + 4] = ct_randf(0.1f, 2.0f);
            //}
            ct_synth_set_formant_id(NODE_ID(s, "osc1"), rand() % 9);
            // ct_synth_calculate_iir_coeff(NODE_ID(s, "filter"),
            //                             ct_randf(300.0f, 2400.0f),
            //                             ct_randf(0.0f,
            //                             0.73f));
            ct_synth_calculate_biquad_coeff(NODE_ID(s, "filter"), PEQ,
                                            // ct_randf(800.0f, 8000.0f),
                                            2400.0f +
                                                sinf(time * 0.3f) * 2320.0f,
                                            -60.0f, ct_randf(0.02f, 0.1f));
            ct_synth_calculate_biquad_coeff(NODE_ID(s, "filter2"), HPF,
                                            // ct_randf(800.0f, 8000.0f),
                                            1600.0f +
                                                sinf(time * 0.25f) * 1500.0f,
                                            -60.0f, ct_randf(0.25f, 2.0f));
            ct_synth_activate_stack(s);
            app->noteID++;
            app->voiceID = (app->voiceID + 1) % app->synth.numStacks;
        }
        app->time = time;
    }

    ct_synth_update_mix_mono_f32(&app->synth, frames, (float *)out);
    return 0;
}
