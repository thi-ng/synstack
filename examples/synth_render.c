#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "synth.h"
#include "osc.h"
#include "adsr.h"
#include "node_ops.h"
#include "delay.h"
#include "formant.h"
#include "iir.h"
#include "foldback.h"
#include "wavfile.h"

#define ITER ((uint32_t)(80.0f * SAMPLE_RATE / AUDIO_BUFFER_SIZE))

const uint8_t scale[] = {36, 38, 40, 43, 45, 60, 50, 52};
const uint8_t scale2[] = {33, 31, 26, 31, 29, 33, 31, 29};

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

int main(int argc, char *argv[]) {
    srand(time(0));
    CT_Synth synth;
    int16_t out[ITER * AUDIO_BUFFER_SIZE];
    uint8_t voiceID = 0;

    ct_synth_init(&synth, 8);
    synth.lfo[0] = ct_synth_osc("lfo1", ct_synth_process_osc_sin, 0.0f,
                                HZ_TO_RAD(0.1f), 0.5f, 1.0f);
    synth.numLFO = 1;

    for (uint8_t i = 0; i < synth.numStacks; i++) {
        CT_DSPStack *s = &synth.stacks[i];
        init_voice(&synth, s, ct_synth_notes[scale[0]]);
        // NODE_ID_STATE(CT_ADSRState, s, "env")->phase = IDLE;
        s->flags = 0;
    }
    ct_synth_collect_stacks(&synth);

    for (uint32_t i = 0, j = 0; i < ITER; i++) {
        if (((i % (ITER / 48)) == 0) && j < 32) {
            float freq = ct_synth_notes[scale[j % 8]];
            float freq2 = ct_synth_notes[scale2[(j % 8)] + 12];
            CT_DSPStack *s = &synth.stacks[voiceID];
            ct_synth_reset_adsr(NODE_ID(s, "env"));
            NODE_ID_STATE(CT_OscState, s, "osc1")->freq = HZ_TO_RAD(freq2);
            NODE_ID_STATE(CT_OscState, s, "osc2")->freq = HZ_TO_RAD(freq);
            // ct_synth_calculate_iir_coeff(NODE_ID(s, "filter"),
            //                            ct_randf(300.0f, 1200.0f), 0.8f);
            ct_synth_activate_stack(s);
            j++;
            voiceID = (voiceID + 1) % synth.numStacks;
        }
        ct_synth_update(&synth);
        ct_synth_mixdown_i16(synth.stackOutputs, &out[i * AUDIO_BUFFER_SIZE], 0,
                             AUDIO_BUFFER_SIZE, synth.numStacks, 1);
    }
    return ct_wavfile_save("sound.wav", out, SAMPLE_RATE, 16, 1,
                           ITER * AUDIO_BUFFER_SIZE);
}
