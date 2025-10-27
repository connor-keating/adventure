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

// Types
struct platform_window
{
  f32 width;
  f32 height;
  void *state;
};


struct clock
{
  f64 secs_per_frame; // The amount of time between frames (seconds).
  f64 ticks_per_sec;  // The frequencey of the performance counter which is fixed at start up and constant (Hz).
  i64 curr;           // The current value of the counter at the start of the frame (ticks).
  i64 prev;           // The value of the counter at the start of the previous frame (ticks).
  f64 delta;          // The amount of seconds that have passed from the beginning of the frame to the end (secs).
};

// TODO: Move controller type info to main.cpp
enum control_state
{
  CONTROL_UP,
  CONTROL_HELD,
  CONTROL_DOWN,
  CONTROL_RELEASED,
};

enum control_bindings
{
  ACTION1,
  ACTION2,
  ACTION3,
  ACTION_COUNT,
};


// Functions
LIBFUNC void             platform_init(arena *a);
LIBFUNC void*            platform_memory_alloc(void *mem_base, size_t mem_size);
LIBFUNC platform_window  platform_window_init();
LIBFUNC void             platform_window_show();
LIBFUNC bool             platform_is_running();
LIBFUNC void             platform_message_process(platform_window *window);
LIBFUNC void             platform_opengl_init();
LIBFUNC void             platform_swapbuffers();


// Ending stuff
#ifdef __cplusplus
}
#endif