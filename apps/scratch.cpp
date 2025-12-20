#include "application.h"

#include "platform.h"

struct appstate
{
  bool is_running;
  platform_window window;
};

global appstate *state;

bool app_is_running()
{
  state->is_running = platform_is_running();
  return state->is_running;
}

void app_init(arena *memory)
{
}

void app_update(arena *a)
{
}