#pragma once

// Dependencies
#include "core.h"
#include "input.h"

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
  f64 secs_per_count; // The inverse frequencey of the performance counter which is fixed at start up and constant (secs/count).
  i64 base;           // The time at the start of the frame.
  i64 curr;           // The current value of the counter at the start of the frame (counts).
  i64 prev;           // The value of the counter at the start of the previous frame (counts).
  i64 stop;           // The amount of time we are paused.
  f64 delta;          // The amount of seconds that have passed from the beginning of the frame to the end (seconds).
  bool paused;        // A flag for pausing the timer.
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
void             platform_init(arena *a);
void*            platform_memory_alloc(void *mem_base, size_t mem_size);
platform_window  platform_window_init();
void             platform_window_show();
void             platform_window_size(platform_window *wind);
void*            platform_window_handle();
void             platform_window_close();
bool             platform_is_running();
void             platform_message_process( platform_window *window, input_state *inputs );
void             platform_opengl_init();
void             platform_swapbuffers();
int              platform_file_exists(const char *filepath);
const char *     platform_file_read(const char *file, arena *scratch, size_t *out_size);
clock            platform_clock_init(f64 fps_target);
i64              platform_clock_time();
void             platform_clock_reset(clock *c);
void             platform_clock_update(clock *c);
void*            platform_dll_load(const char *filepath);
void*             platform_dll_func_load(void *dll, const char *func_name);

// TODO: Delete this, see if you can use the C++ tinyobj
void             platform_file_data(void* ctx, const char* filename, const int is_mtl, const char* obj_filename, char** data, size_t* len);
