#include "demo_common.h"

static const uint8_t scale[] = {36, 40, 43, 45, 60, 48, 52};
static const float pitch[]   = {0.25f, 0.5f, 1.0f};

static CT_Smush rnd;

static void init_voice(CTSS_Synth *synth, CTSS_DSPStack *stack, float freq);
static int render_synth(const void *in,
                        void *out,
                        unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status,
                        void *data);

int main(int argc, char *argv[]) {
  AppState app;

  ct_smush_init(&rnd, 0xdecafbad);
  ctss_preinit_osc_formant();
  ctss_init(&app.synth, 8);

  ctss_add_global_lfo(&app.synth, ctss_osc("lfo1", ctss_process_osc_sin, 0.0f,
                                           0.01f, 0.5f, 1.0f));

  for (uint8_t i = 0; i < app.synth.numStacks; i++) {
    init_voice(&app.synth, &app.synth.stacks[i], ctss_notes[scale[0]]);
  }

  app.pitch    = -18;
  app.callback = render_synth;
  app.handler  = demo_key_handler;
  app.channels = 1;
  return demo(&app);
}

static void init_voice(CTSS_Synth *synth, CTSS_DSPStack *stack, float freq) {
  ctss_init_stack(stack);
  CTSS_DSPNode *env = ctss_adsr("env", synth->lfo[0]);
  ctss_configure_adsr(env, 2.0f, 0.6f, 0.01f, 1.0f, 0.8f, false);
  CTSS_DSPNode *osc1 =
      ctss_osc_formant_id("osc1", 0, freq, 0.15f, 0.0f, 0.001f);
  CTSS_DSPNode *sum = ctss_op2("sum", osc1, env, ctss_process_mult);
  CTSS_DSPNode *filter =
      ctss_filter_iir("filter", IIR_HP, sum, NULL,
                      ct_smush_minmax(&rnd, 300.0f, 1200.0f), 0.8f);
  CTSS_DSPNode *delay =
      ctss_delay("delay", filter, (uint32_t)(SAMPLE_RATE * 3 / 8), 0.9f, 1);
  CTSS_DSPNode *nodes[] = {env, osc1, sum, filter, delay};
  ctss_build_stack(stack, nodes, 5);
}

static int render_synth(const void *in,
                        void *out,
                        unsigned long frames,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags status,
                        void *data) {
  AppState *app = (AppState *)data;
  app->time     = fmodf((float)timeInfo->currentTime, 0.5f);
  if (app->time < 0.006f) {
    float freq = ctss_notes[scale[app->noteID % 7] + app->pitch] *
                 pitch[ct_smush(&rnd) % 3];
    CTSS_DSPStack *s = &app->synth.stacks[app->voiceID];
    ctss_reset_adsr(NODE_ID(s, "env"));
    CTSS_FormantOsc *osc = NODE_ID_STATE(CTSS_FormantOsc, s, "osc1");
    osc->freq            = HZ_TO_RAD(freq);
    ctss_set_formant_id(NODE_ID(s, "osc1"), ct_smush(&rnd) % 9);
    ctss_calculate_iir_coeff(NODE_ID(s, "filter"),
                             ct_smush_minmax(&rnd, 300.0f, 5000.0f), 0.7f);
    ctss_activate_stack(s);
    app->noteID++;
    app->voiceID = (app->voiceID + 1) % app->synth.numStacks;
  }

  ctss_update_mix_mono_f32(&app->synth, ctss_mixdown_f32, frames, (float *)out);
  return 0;
}
