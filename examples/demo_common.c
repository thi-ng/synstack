#include "demo_common.h"

uint8_t demo(AppState *app) {
  initscr();
  start_color();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  PaStream *stream;
  PaError err;

  err = Pa_Initialize();
  if (err != paNoError) {
    goto error;
  }

  ctss_collect_stacks(&app->synth);
  app->voiceID = 0;
  app->noteID  = 0;
  app->time    = 0;

  err = Pa_OpenDefaultStream(&stream, 0, /* no input channels */
                             app->channels, paFloat32, SAMPLE_RATE,
                             256, /* frames per buffer */
                             app->callback, app);
  if (err != paNoError) {
    goto error;
  }

  err = Pa_StartStream(stream);
  if (err != paNoError) {
    goto error;
  }

  int ch;
  mvprintw(0, 0, "ready...");
  // addch((char)0x2588);
  // addch(' '|A_REVERSE);
  while ((ch = getch()) != 'q') {
    app->handler(app, ch);
    mvprintw(2, 2, "pitch: %d  ", app->pitch);
  }

  err = Pa_StopStream(stream);
  if (err != paNoError) {
    goto error;
  }
  err = Pa_CloseStream(stream);
  if (err != paNoError) {
    goto error;
  }
  Pa_Terminate();
  endwin();
  return err;

error:
  Pa_Terminate();
  endwin();
  fprintf(stderr, "An error occured while using the portaudio stream\n");
  fprintf(stderr, "Error number: %d\n", err);
  fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
  return err;
}

void demo_key_handler(AppState *app, char ch) {
  switch (ch) {
    case '-':
      for (uint8_t i = 0; i < app->synth.numStacks; i++) {
        CTSS_DelayState *s =
            (CTSS_DelayState *)(NODE_ID(&app->synth.stacks[i], "delay")->state);
        s->feedback -= 0.1f;
        if (s->feedback <= 0) {
          s->feedback = 0;
        }
      }
      mvprintw(
          1, 2, "feedback: %f",
          ((CTSS_DelayState *)(NODE_ID(&app->synth.stacks[0], "delay")->state))
              ->feedback);
      break;
    case '=':
      for (uint8_t i = 0; i < app->synth.numStacks; i++) {
        CTSS_DelayState *s =
            (CTSS_DelayState *)(NODE_ID(&app->synth.stacks[i], "delay")->state);
        s->feedback += 0.1f;
        if (s->feedback >= 1.0) {
          s->feedback = 1.0;
        }
      }
      mvprintw(
          1, 2, "feedback: %f",
          ((CTSS_DelayState *)(NODE_ID(&app->synth.stacks[0], "delay")->state))
              ->feedback);
      break;
    case '[':
      app->pitch -= 2;
      break;
    case ']':
      app->pitch += 2;
      break;
    case ';':
      app->pitch = (app->pitch >= -20 ? app->pitch - 12 : app->pitch);
      break;
    case '\'':
      app->pitch = (app->pitch <= 24 ? app->pitch + 12 : app->pitch);
      break;
  }
}
