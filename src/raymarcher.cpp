#include "application.h"

#include "platform.h"


struct appstate
{
  bool is_running;
  platform_window window;
  input_state inputs[KEY_COUNT];
};

global appstate *state;


void app_init( arena *memory )
{
  // Create internal global state
  state = arena_push_struct( memory, appstate );
  state->is_running = false;
  // Start the platform layer
  platform_init( memory );
  // Create a window for the application
  state->window = platform_window_init();
  platform_window_show();
}

bool app_is_running()
{
  state->is_running = platform_is_running();
  return state->is_running;
}

void app_update( arena *memory )
{
  // Empty message queue
  platform_message_process( &state->window, state->inputs );
  if ( state->inputs[KEY_ESCAPE] == INPUT_DOWN )
  {
    platform_window_close();
  }
}
