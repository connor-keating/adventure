
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
  platform_window window = {};

  // Allocate all program memory upfront.
  void *memory_base = 0;
  size_t memory_size = (size_t) Gigabytes(1);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena memory;
  arena_init(&memory, raw_memory, memory_size);

  window_init(&window);

  // Initialize renderer
  render_state renderer = render_init(&window);

  // Set up point program
  render_program prog;
  // point_setup(&memory, &prog);
  cube_setup(&memory, &prog);
  arena_free_all(&memory);

  i32 display_flags = SW_SHOW;
  ShowWindow(window.handle, display_flags);
  UpdateWindow(window.handle);
  is_running = true;
  while (is_running)
  {
    message_process(window.handle);

    // Check window dimensions
    /*
      1. Check the handle is properly set in window.
      2. 
    */
    window_size_get(&window);
    // Initialize frame
    frame_init(&renderer);
    // Set uniform
    g_angle += 0.01f; // tweak speed here (radians per frame)
    float fov_deg = 45.0f;            // pick your FOV
    float width = window.width;
    float height = window.height; // TODO: Don't hardcode this
    float aspect   = width / height; // keep updated on resize
    uniform_set(&prog, g_angle, fov_deg, aspect);
    // Draw call
    draw_lines(&prog);
    // Finalize and draw frame
    frame_render(&renderer);

  }

  // close and release all existing COM objects
  render_close(&renderer, &prog);


  return 0;
}

