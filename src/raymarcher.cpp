#include "application.h"

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

void app_init( arena *memory )
{
  // Create internal global state
  state = arena_push_struct( memory, appstate );
  state->is_running = false;
  // Start the platform layer
  platform_init( memory );
  // Create a window for the application
  state->window = platform_window_init();
  // Initialize renderer
  render_init( memory );
  // Application startup complete, show the window.
  platform_window_show();
}

void app_update( arena *memory )
{
  // Empty message queue
  platform_message_process( &state->window, state->inputs );
  if ( state->inputs[KEY_ESCAPE] == INPUT_DOWN )
  {
    platform_window_close();
  }
  frame_init();
  frame_render();
}
