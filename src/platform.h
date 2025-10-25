#pragma once

// Dependencies
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

struct clock
{
  f64 secs_per_frame; // The amount of time between frames (seconds).
  f64 ticks_per_sec;  // The frequencey of the performance counter which is fixed at start up and constant (Hz).
  i64 curr;           // The current value of the counter at the start of the frame (ticks).
  i64 prev;           // The value of the counter at the start of the previous frame (ticks).
  f64 delta;          // The amount of seconds that have passed from the beginning of the frame to the end (secs).
};


// Functions
LIBFUNC void  platform_status();
LIBFUNC void* platform_memory_alloc(void *mem_base, size_t mem_size);

// Ending stuff
#ifdef __cplusplus
}
#endif