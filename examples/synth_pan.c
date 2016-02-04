#include "demo_common.h"

const uint8_t scale[] = {36, 40, 43, 45, 60, 48, 52, 55};
const uint8_t scale2[] = {33, 31, 26, 31, 29, 33, 31, 29};
const float pitch[] = {0.5f, 1.0f, 1.0f};

static void init_voice(CTSS_Synth *synth, CTSS_DSPStack *stack, float freq);
static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data);

int main(int argc, char *argv[]) {
    AppState app;
    srand(time(0));
    ctss_init(&app.synth, 32);
    app.synth.lfo[0] = ctss_osc("lfo1", ctss_process_osc_sin, 0.0f,
                                HZ_TO_RAD(0.125f), 0.5f, 1.2f);
    app.synth.lfo[1] = ctss_osc("lfo2", ctss_process_osc_sin, 0.0f,
                                HZ_TO_RAD(0.25f), 0.495f, 0.5f);
    app.synth.numLFO = 2;
    for (uint8_t i = 0; i < app.synth.numStacks; i++) {
        init_voice(&app.synth, &app.synth.stacks[i], ctss_notes[scale[0]]);
    }
    app.pitch = -8;
    app.callback = render_synth;
    app.handler = demo_key_handler;
    app.channels = 2;
    return demo(&app);
}

static void init_voice(CTSS_Synth *synth, CTSS_DSPStack *stack, float freq) {
    ctss_init_stack(stack);
    CTSS_DSPNode *env =
        ctss_adsr("env", synth->lfo[0], 0.0005f, 0.02f, 0.3f, 1.0f, 0.5f);
    CTSS_DSPNode *osc1 = ctss_osc_pluck("osc1", freq, 0.001f, 0.6f, 0.0f);
    CTSS_DSPNode *sum = ctss_op2("sum", osc1, env, ctss_process_mult);
    CTSS_DSPNode *filter =
        ctss_filter_iir("filter", IIR_LP, sum, NULL, 18000.0f, 0.1f);
    CTSS_DSPNode *pan = ctss_panning("pan", filter, synth->lfo[1], 0.0f);
    CTSS_DSPNode *delay = ctss_delay(
        "delay", pan, (uint32_t)(SAMPLE_RATE * 0.25f * 1.5f), 0.3f, 2);

    CTSS_DSPNode *nodes[] = {env, osc1, sum, filter, pan, delay};
    ctss_build_stack(stack, nodes, 6);
}

static int render_synth(const void *in, void *out, unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status, void *data) {
    AppState *app = (AppState *)data;
    float time = (float)timeInfo->currentTime;
    float ft = fmodf(time, 0.25f);
    if (ft < 0.008) {
        float freq = ctss_notes[scale[app->noteID % 8] + app->pitch];
        CTSS_DSPStack *s = &app->synth.stacks[app->voiceID];
        ctss_reset_adsr(NODE_ID(s, "env"));
        freq *= pitch[rand() % 3];
        ctss_reset_pluck(NODE_ID(s, "osc1"), freq, 0.01f,
                         ct_randf(0.05f, 0.95f));
        NODE_ID_STATE(CTSS_PluckOsc, s, "osc1")->gain = ct_randf(0.15f, 0.7f);
        ctss_calculate_iir_coeff(NODE_ID(s, "filter"), 18000.0f, 0.1f);
        ctss_activate_stack(s);
        app->noteID++;
        app->voiceID = (app->voiceID + 1) % app->synth.numStacks;
    }
    ft = fmodf(time, 0.375f);
    if (ft < 0.008) {
        float freq = ctss_notes[scale2[app->noteID % 8] + app->pitch + 7];
        CTSS_DSPStack *s = &app->synth.stacks[app->voiceID];
        ctss_reset_adsr(NODE_ID(s, "env"));
        ctss_reset_pluck(NODE_ID(s, "osc1"), freq, 0.001f,
                         ct_randf(0.05f, 0.95f));
        NODE_ID_STATE(CTSS_PluckOsc, s, "osc1")->gain = ct_randf(0.6f, 0.9f);
        ctss_calculate_iir_coeff(NODE_ID(s, "filter"),
                                 freq * ct_randf(0.9f, 1.9f),
                                 ct_randf(0.0f, 0.6f));
        ctss_activate_stack(s);
        app->noteID++;
        app->voiceID = (app->voiceID + 1) % app->synth.numStacks;
    }

    ctss_update_mix_stereo_f32(&app->synth, ctss_mixdown_f32, frames,
                               (float *)out);
    return 0;
}
