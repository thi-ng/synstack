#include "demo_common.h"

const uint8_t scale[] = {36, 40, 43, 45, 60, 48, 52, 55};
const float pitch[] = {0.25f, 0.5f, 1.0f, 1.0f};

static void init_voice(CT_Synth *synth, CT_DSPStack *stack, float freq);
static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data);

int main(int argc, char *argv[]) {
    AppState app;

    srand(time(0));

    ct_synth_preinit_osc_formant();
    ct_synth_init(&app.synth, 8);

    app.synth.lfo[0] = ct_synth_osc("lfo1", ct_synth_process_osc_sin, 0.0f,
                                    HZ_TO_RAD(0.125f), 0.9f, 1.0f);
    app.synth.numLFO = 1;

    for (uint8_t i = 0; i < app.synth.numStacks; i++) {
        init_voice(&app.synth, &app.synth.stacks[i], ct_synth_notes[scale[0]]);
    }

    app.pitch = 28;
    app.callback = render_synth;
    app.handler = demo_key_handler;
    app.channels = 1;
    return demo(&app);
}

static void init_voice(CT_Synth *synth, CT_DSPStack *stack, float freq) {
    ct_synth_init_stack(stack);
    CT_DSPNode *env =
        ct_synth_adsr("env", synth->lfo[0], 0.01f, 0.05f, 0.3f, 1.0f, 0.15f);
    CT_DSPNode *env2 =
        ct_synth_adsr("env2", synth->lfo[0], 0.005f, 0.1f, 0.02f, 1.0f, 0.99f);
    CT_DSPNode *lfo = ct_synth_osc("lfoPWM", ct_synth_process_osc_sin, 0.0f,
                                   HZ_TO_RAD(3.33f), 0.45f, 0.5f);
    CT_DSPNode *osc1 =
        ct_synth_osc("osc1", ct_synth_process_osc_pblep, 0, freq, 0.05f, 0);
    CT_DSPNode *osc2 = ct_synth_osc("osc2", ct_synth_process_osc_pblep, 0,
                                    freq * 0.501f, 0.05f, 0);
    CT_DSPNode *osc3 =
        ct_synth_osc("osc3", ct_synth_process_osc_sin, 0.0f, freq, 0.05f, 0.0f);
    ct_synth_set_osc_pblep(osc1, ct_osc_pblep_pwm);
    ct_synth_set_osc_pblep(osc2, ct_osc_pblep_pwm);
    ct_synth_set_osc_lfo(osc1, lfo, 1.0f);
    ct_synth_set_osc_lfo(osc2, lfo, 1.0f);
    CT_DSPNode *sum =
        ct_synth_op4("sum", osc1, env, osc2, env, ct_synth_process_madd);
    CT_DSPNode *sum2 = ct_synth_op2("sum2", osc3, env2, ct_synth_process_mult);
    CT_DSPNode *filter = ct_synth_filter_iir("filter", IIR_LP, sum, NULL,
                                             ct_randf(300.0f, 1200.0f), 0.0f);
    CT_DSPNode *delay = ct_synth_delay(
        "delay", filter, (uint32_t)(SAMPLE_RATE * 0.25f * 1.5f), 0.5f, 1);
    CT_DSPNode *sum3 = ct_synth_op2("sum3", delay, sum2, ct_synth_process_sum);
    CT_DSPNode *nodes[] = {env, env2, lfo,    osc1,  osc2, osc3,
                           sum, sum2, filter, delay, sum3};
    ct_synth_build_stack(stack, nodes, 11);
}

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    AppState *app = (AppState *)data;
    float time = (float)timeInfo->currentTime;
    float ft = fmodf(time, 0.25f);
    if (ft < 0.008) {
        float freq = ct_synth_notes[scale[app->noteID % 8] + app->pitch];
        float freq2 = ct_synth_notes[(scale[app->noteID % 8] % 12) + 7 +
                                     (app->pitch % 12)];
        freq *= pitch[rand() & 3];
        CT_DSPStack *s = &app->synth.stacks[app->voiceID];
        ct_synth_reset_adsr(NODE_ID(s, "env"));
        NODE_ID_STATE(CT_OscState, s, "osc1")->freq = HZ_TO_RAD(freq);
        NODE_ID_STATE(CT_OscState, s, "osc2")->freq = HZ_TO_RAD(freq * 0.501f);
        if (time - app->time > 0.249) {
            ct_synth_reset_adsr(NODE_ID(s, "env2"));
            NODE_ID_STATE(CT_OscState, s, "osc3")->freq = HZ_TO_RAD(freq2);
            app->time = time;
        }
        ct_synth_calculate_iir_coeff(NODE_ID(s, "filter"),
                                     ct_randf(100.0f, 5000.0f),
                                     ct_randf(0.1f, 0.6f));
        ct_synth_activate_stack(s);
        app->noteID++;
        app->voiceID = (app->voiceID + 1) % app->synth.numStacks;
        app->isNewNote = 0;
    }

    ct_synth_update_mix_mono_f32(&app->synth, frames, (float *)out);
    return 0;
}
