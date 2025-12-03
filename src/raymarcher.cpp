#include "application.h"

#include "platform.h"


void app_init( arena *memory )
{
  // Start the platform layer
  platform_init( memory );
}
