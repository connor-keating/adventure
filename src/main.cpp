
#include <stdio.h>



#include "core.cpp"
#include "linalg.cpp"

// Globals
global bool is_running;

void handle_input()
{
  printf("Handling Input");
}

// Merge source code into single translation unit
#ifdef PLATFORM_WINDOWS
#include "platform_win32.cpp"
#endif

#ifdef _DX11
#include "render_dx11.cpp"
#elif _OPENGL
#include "render_opengl.cpp"
#endif


// TODO: Why does app crash when I share it with discord?


void input_reset(control_state *input_state)
{
  for (i32 i=0; i < ACTION_COUNT; ++i)
  {
    // Get the state, if it is released make it up (idle)
    if (input_state[i] == CONTROL_RELEASED)
    {
      input_state[i] = CONTROL_UP;
      printf("Action %d, up\n", i+1);
    }
    else if (input_state[i] == CONTROL_DOWN)
    {
      input_state[i] = CONTROL_HELD;
      printf("Action %d, held\n", i+1);
    }
  }
}


int main(int argc, char **argv)
{
  // Arrays indexed directly by control_bindings values

  platform_window window = {};

  // Allocate all program memory upfront.
  void *memory_base = 0;
  size_t memory_size = (size_t) Gigabytes(1);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena memory;
  arena_init(&memory, raw_memory, memory_size);

  window_init(&window);

  // Application clock
  f64 fps_target = 60;                // The amount of frames presented in a second.
  clock app_clock = clock_init(fps_target);

  // Initialize input map
  i32           input_map[ACTION_COUNT];
  control_state input_state[ACTION_COUNT];

  // Set the bindings
  platform_control_set(input_map);

  // Initialize renderer
  render_state renderer = render_init(&window);

  // Set up point program
  render_program prog_points = point_setup(&memory);
  render_program prog = cube_setup(&memory);
  render_program prog2 = instance_setup(&memory);
  arena_free_all(&memory);

  // Initialize UI buffer
  render_program prog_ui = ui_init(&renderer, &memory);
  arena_free_all(&memory);

  // Set up the angular speed variable for the rotation
  f32 angle_velocity = PI/4.0f;
  f32 angle = 0.0f;

  // Instance shader toggle
  bool toggle = 1;

  // VSynch
  wglSwapIntervalEXT(0); // 1 is on 0 is off.
  // Show window
  i32 display_flags = SW_SHOW;
  ShowWindow(window.handle, display_flags);
  UpdateWindow(window.handle);
  is_running = true;
  while (is_running)
  {
    // Frame start
    clock_update(&app_clock);
    message_process(window.handle, input_map, input_state);

    // Check window dimensions
    window_size_get(&window);
    // Set render window dimensions (for now the whole canvas)
    renderer.width = window.width;
    renderer.height = window.height;
    // Initialize frame
    frame_init(&renderer);

    // Draw the UI
    draw_ui(&prog_ui);

    // Draw the editor
    // draw_points(&prog_points);

    // wrap angle so it doesn't explode
    angle += angle_velocity * app_clock.delta; // rad += (rad/s)*s
    if (angle > 2.0*PI) angle -= 2.0*PI;
    f32 fov_deg = 45.0f;            // pick your FOV
    f32 aspect  = window.width / (window.height + 0.000001); // keep updated on resize
    uniform_set(&prog, angle, fov_deg, aspect);

    // Set color
    if (input_state[ACTION1] == CONTROL_DOWN)
    {
      toggle = !toggle;
    }
    f32 color[3];
    if (toggle)
    {
      color[0] = 0.0f;
      color[1] = 1.0f;
      color[2] = 0.0f;
    }
    else 
    {
      color[0] = 1.0f;
      color[1] = 0.0f;
      color[2] = 0.0f;
    }
    // glUseProgram(prog2.shader_program);
    // GLint loc = glGetUniformLocation( prog2.shader_program, "mycolor");
    // glUniform3fv( loc, 1, color);

    // Draw spinning cube
    // draw_lines(&prog);

    // Draw instance cube
    // uniform_set(&prog2, angle, fov_deg, aspect);
    // draw_lines_instanced(&prog2);
    // Finalize and draw frame
    frame_render(&renderer);
    // Reset input after processing everything
    input_reset(input_state);
  }

  // close and release all existing COM objects
  // program_close(&prog);
  // program_close(&prog2);
  render_close(&renderer);


  return 0;
}

