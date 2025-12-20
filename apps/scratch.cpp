#include "application.h"

#include "linalg.cpp"
#include "input.h"
#include "platform.h"
#include "render.h"

struct appstate
{
  bool is_running;
  platform_window window;
  input_state inputs[KEY_COUNT];
};

global appstate *state;

bool app_is_running()
{
  state->is_running = platform_is_running();
  return state->is_running;
}

void app_init(arena *memory)
{
  // Create internal global state
  state = arena_push_struct(memory, appstate);
  state->is_running = false;
  // Start the platform layer
  platform_init(memory);
  // Create a window for the application
  state->window = platform_window_init();
  // Initialize renderer
  render_init(memory);
  platform_window_show();
}

void app_update(arena *a)
{
  platform_message_process(&state->window, state->inputs);
  if (state->inputs[KEY_ESCAPE] == INPUT_DOWN)
  {
    platform_window_close();
  }
  // fvec4 frame_background = fvec4_init( 0.0f, 0.325f, 0.282f, 1.0f );
  fvec4 frame_background = fvec4_init(0.0f, 0.0f, 0.0f, 0.0f);

  frame_init(frame_background.array);
  frame_render();
}