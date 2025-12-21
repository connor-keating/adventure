#include "application.h"

#include "linalg.cpp"
#include "input.h"
#include "platform.h"
#include "render.h"
#include "app_data.h"

#define MAX_COUNT_SHADERS 100
#define MAX_COUNT_VERTEX  1000


struct appstate
{
  bool             is_running;
  platform_window  window;
  input_state      inputs[KEY_COUNT];
  u64              shader[MAX_COUNT_SHADERS];
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
  render_data_init( memory, MAX_COUNT_SHADERS );
  // Shaders
  state->shader[0] = shader_init( memory );
  shader_load( state->shader[0], VERTEX, "shaders/tri.hlsl", "VSMain", "vs_5_0");
  shader_load( state->shader[0], PIXEL,  "shaders/tri.hlsl", "PSMain", "ps_5_0");
  // Start the window
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
  fvec4 frame_background = fvec4_init(0.0f, 0.0f, 0.0f, 1.0f);

  frame_init(frame_background.array);
  shader_set( state->shader[0] );
  render_draw( 3 );
  frame_render();
}