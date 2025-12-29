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

LIBFUNC bool app_is_running();
LIBFUNC void app_init( arena *a );
LIBFUNC void app_update( arena *a );

// Ending stuff
#ifdef __cplusplus
}
#endif