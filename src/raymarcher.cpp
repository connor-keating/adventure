#include "application.h"

#include "platform.h"


struct appstate
{
  bool is_running;
  platform_window window;
};

global appstate *state;


void app_init( arena *memory )
{
  // Create internal global state
  state = arena_push_struct( memory, appstate );
  state->is_running = false;
  // Start the platform layer
  platform_init( memory );
}

bool app_is_running()
{
  return state->is_running;
}

void app_update( arena *memory )
{
}
