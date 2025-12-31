#include "application.h"

int main(int argc, char **argv)
{
  arena memory = app_init();
  while ( app_is_running() )
  {
    app_update( &memory );
  }

  return 0;
}
