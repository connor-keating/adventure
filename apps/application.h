#pragma once

#include "core.h"

// Macros
#ifdef PLATFORM_WINDOWS
  #ifdef _EXPORT
    #define LIBFUNC __declspec(dllexport)
  #else
    #define LIBFUNC __declspec(dllimport)
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBFUNC arena app_init();
LIBFUNC void  app_update( arena *a );
LIBFUNC bool  app_is_running();

// Ending stuff
#ifdef __cplusplus
}
#endif