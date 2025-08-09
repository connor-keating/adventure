
#include <stdio.h>



#include "core.cpp"
#include "linalg.cpp"

// Globals
global bool is_running;
float g_angle = 0.0f;


// Merge source code into single translation unit
#ifdef PLATFORM_WINDOWS
#include "platform_win32.cpp"
#endif

#ifdef _DX11
#include "render_dx11.cpp"
#elif _OPENGL
#include "render_opengl.cpp"
#endif




int main(int argc, char **argv)
{
  HWND handle;
  HINSTANCE instance;

  // Allocate all program memory upfront.
  void *memory_base = 0;
  size_t memory_size = (size_t) Gigabytes(1);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena memory;
  arena_init(&memory, raw_memory, memory_size);

  window_init(&handle, &instance);

  // Initialize renderer
  render_state renderer = render_init(&handle, &instance);

  // Set up point program
  render_program prog;
  // point_setup(&memory, &prog);
  cube_setup(&memory, &prog);
  arena_free_all(&memory);

  i32 display_flags = SW_SHOW;
  ShowWindow(handle, display_flags);
  UpdateWindow(handle);
  is_running = true;
  while (is_running)
  {
    message_process(handle);

    // Render frame
    frame_init(&renderer);
    draw_triangles(&prog);
    frame_render(&renderer);

  }

  // close and release all existing COM objects
  render_close(&renderer, &prog);


  return 0;
}

