
#include <stdio.h>



#include "core.cpp"

// Globals
global bool is_running;


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

  window_init(&handle, &instance);

  // Initialize renderer
  render_state renderer = render_init(&handle, &instance);

  i32 display_flags = SW_SHOW;
  ShowWindow(handle, display_flags);
  UpdateWindow(handle);
  is_running = true;
  while (is_running)
  {
    message_process(handle);

    // Render frame
    frame_render(&renderer);

  }

  // close and release all existing COM objects
  render_close(&renderer);


  return 0;
}

