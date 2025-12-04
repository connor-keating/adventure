#include "application.h"

#include "platform.h"


void app_init( arena *memory )
{
  // Start the platform layer
  platform_init( memory );
}

bool app_is_running()
{
  return false;
}

void app_update( arena *memory )
{
}
