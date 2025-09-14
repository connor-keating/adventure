
#include <stdio.h>



#include "core.cpp"
#include "linalg.cpp"

// Application data types
struct vertex
{
  fvec3 pos;
};


struct mesh
{
  vertex *vertices;
  u32 *indices;
  u32 vert_count;
  u32 index_count;
};

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

// Application layers
#include "text.cpp"

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
  // Allocate all program memory upfront.
  void *memory_base = 0;
  size_t memory_size = (size_t) Gigabytes(1);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena memory;
  arena_init(&memory, raw_memory, memory_size);

  // Load application assets
  text_atlas_init();


  // Create a window for the application
  platform_window window = {};
  window_init(&window);

  // Application clock
  f64 fps_target = 60;                // The amount of frames presented in a second.
  clock app_clock = platform_clock_init(fps_target);

  // Initialize input map
  i32           input_map[ACTION_COUNT];
  control_state input_state[ACTION_COUNT];

  // Set the bindings
  platform_control_set(input_map);

  // Initialize renderer
  render_state renderer = render_init(&window);

  // Set up point program
  // render_program prog_points = point_setup(&memory);
  // render_program prog = cube_setup(&memory);
  render_buffer instance_buffer = instance_setup(&memory);
  u32 instance_program = render_program_init( &memory, "shaders\\instance.vert", "shaders\\instance.frag");
  // arena_free_all(&memory);

  // Initialize UI buffer
  f32 verts[] = {
    // Position          UV coords
    -1.0f, -1.0f, 0.0f, //  0.0f, 0.0f, // Lower left
     1.0f, -1.0f, 0.0f, //  1.0f, 0.0f, // Lower right
    -1.0f,  1.0f, 0.0f, //  0.0f, 1.0f, // Upper left
     1.0f,  1.0f, 0.0f, //  1.0f, 1.0f, // Upper right
  };
  render_buffer ui_buffer = render_buffer_init((void*)verts, sizeof(verts));
  ui_init(&renderer, &memory, ui_buffer);
  u32 ui_program = render_program_init(&memory, "shaders\\text.vert", "shaders\\text.frag");
  arena_free_all(&memory);

  // Read in model data
  mesh teapot_model = read_obj("assets\\teapot.obj", &memory);
  size_t teapot_buffer_size = sizeof(teapot_model.vertices[0]) * teapot_model.vert_count;
  render_buffer teapot_buffer = render_buffer_init((void*)teapot_model.vertices, teapot_buffer_size);
  render_buffer_attribute(
    teapot_buffer,
    0, 
    ARRAY_COUNT(teapot_model.vertices[0].pos.array),
    ARRAY_COUNT(teapot_model.vertices[0].pos.array) * sizeof(teapot_model.vertices[0].pos.array[0]),
    (void *)0
  );
  u32 teapot_program = render_program_init(&memory, "shaders\\points.vert", "shaders\\points.frag");

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
    platform_clock_update(&app_clock);
    message_process(window.handle, input_map, input_state);

    // Check window dimensions
    window_size_get(&window);
    // Set render window dimensions (for now the whole canvas)
    renderer.width = window.width;
    renderer.height = window.height;
    // Initialize frame
    frame_init(&renderer);

    // Draw the UI
    f32 ui_scale_x = 0.10 * renderer.width;
    f32 ui_scale_y = 0.10 * renderer.height;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(renderer.width * 0.5, renderer.height * 0.5, 0.0f));
    model = glm::scale(model, glm::vec3(ui_scale_x, ui_scale_y, 1.0f));
    glm::mat4 ortho = glm::ortho(0.0f, renderer.width, 0.0f, renderer.height, -1.0f, 1.0f);
    glm::mat4 proj = ortho * model;
    uniform_set_mat4(ui_program, "proj", &proj[0][0]);
    // draw_ui(ui_buffer, ui_program);

    glm::mat4 identity = glm::mat4(1.0f);
    // Create view and projection matrix
    angle += angle_velocity * app_clock.delta; // rad += (rad/s)*s
    if (angle > 2.0*PI) angle -= 2.0*PI;
    f32 fov_deg = 45.0f;            // pick your FOV
    f32 aspect  = window.width / (window.height + 0.000001); // keep updated on resize
    glm::mat4 perspective_model = glm::mat4(1.0f);
    glm::vec3 rotation_axis_norm = glm::vec3(0,1,0);
    perspective_model = glm::rotate(perspective_model, angle, rotation_axis_norm);
    // look at
    glm::vec3 camera_pos    = glm::vec3(0, 5.0f, 10.0f);
    glm::vec3 camera_target = glm::vec3(0,0,0);
    glm::vec3 camera_up     = glm::vec3(0,1,0);
    glm::mat4 view = glm::lookAt(camera_pos, camera_target, camera_up);
    // perspective
    f32 fov_rad = glm::radians(fov_deg);
    f32 znear = 0.1f;
    f32 zfar = 100.0f;
    glm::mat4 perspective_proj = glm::perspective(fov_rad, aspect, znear, zfar);
    glm::mat4 mvp = perspective_proj * view * perspective_model;
    // Draw the model
    uniform_set_mat4(teapot_program, "proj", &mvp[0][0]);
    draw_points(teapot_buffer, teapot_program, teapot_model.vert_count);

    // Draw the editor
    // draw_points(&prog_points);

    // wrap angle so it doesn't explode

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
    glUseProgram(instance_program);
    GLint loc = glGetUniformLocation( instance_program, "mycolor");
    glUniform3fv( loc, 1, color);

    // Draw spinning cube
    // draw_lines(&prog);

    // Draw instance cube
    uniform_set(instance_program, angle, fov_deg, aspect);
    // draw_lines_instanced(instance_buffer, instance_program);

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

