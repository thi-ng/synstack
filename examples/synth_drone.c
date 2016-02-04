#include "demo_common.h"

const uint8_t scale[] = {36, 40, 43, 45, 60, 48, 52};
const float pitch[] = {0.25f, 0.5f, 1.0f};

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
        ct_synth_osc("lfo1", ct_synth_process_osc_sin, 0.0f, 0.01f, 0.5f, 1.0f);
    app.synth.numLFO = 1;

    for (uint8_t i = 0; i < app.synth.numStacks; i++) {
        init_voice(&app.synth, &app.synth.stacks[i], ct_synth_notes[scale[0]]);
    }

    app.pitch = 0;
    app.callback = render_synth;
    app.handler = demo_key_handler;
    app.channels = 1;
    return demo(&app);
}

static void init_voice(CT_Synth *synth, CT_DSPStack *stack, float freq) {
    ct_synth_init_stack(stack);
    CT_DSPNode *env =
        ct_synth_adsr("env", synth->lfo[0], 2.0f, 0.6f, 0.01f, 1.0f, 0.8f);
    CT_DSPNode *osc1 =
        ct_synth_osc_formant_id("osc1", 0, freq, 0.15f, 0.0f, 0.001f);
    CT_DSPNode *sum = ct_synth_op2("sum", osc1, env, ct_synth_process_mult);
    CT_DSPNode *filter = ct_synth_filter_iir("filter", IIR_HP, sum, NULL,
                                             ct_randf(300.0f, 1200.0f), 0.8f);
    CT_DSPNode *delay = ct_synth_delay(
        "delay", filter, (uint32_t)(SAMPLE_RATE * 3 / 8), 0.0f, 1);
    CT_DSPNode *nodes[] = {env, osc1, sum, filter, delay};
    ct_synth_build_stack(stack, nodes, 5);
}

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    AppState *app = (AppState *)data;
    app->time = fmodf((float)timeInfo->currentTime, 0.5f);
    if (app->time < 0.006f) {
        float freq = ct_synth_notes[scale[app->noteID % 7] + app->pitch] *
                     pitch[rand() % 3];
        CT_DSPStack *s = &app->synth.stacks[app->voiceID];
        ct_synth_reset_adsr(NODE_ID(s, "env"));
        CT_FormantOsc *osc = NODE_ID_STATE(CT_FormantOsc, s, "osc1");
        osc->freq = HZ_TO_RAD(freq);
        ct_synth_set_formant_id(NODE_ID(s, "osc1"), rand() % 9);
        ct_synth_calculate_iir_coeff(NODE_ID(s, "filter"),
                                     ct_randf(300.0f, 5000.0f), 0.7f);
        ct_synth_activate_stack(s);
        app->noteID++;
        app->voiceID = (app->voiceID + 1) % app->synth.numStacks;
    }

    ct_synth_update_mix_mono_f32(&app->synth, frames, (float *)out);
    return 0;
}
