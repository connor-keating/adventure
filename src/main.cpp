
#include <stdio.h>



#include "core.cpp"
#include "linalg.cpp"


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
#include "data3d.cpp"

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
#if _DEBUG
  void *memory_base = (void*)Terabytes(2);
#else
  void *memory_base = 0;
#endif
  size_t memory_size = (size_t) Gigabytes(1);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena memory = arena_init(raw_memory, memory_size);

  // Init CPU buffers
  // Scratch arena that can be freed frequently.
  u32 scratch_max = Megabytes(20);
  arena scratch = subarena_init(&memory, scratch_max);
  // Render buffer that contains line data
  u32 lines_max = 100000;
  arena vert_buffer_lines = subarena_init(&memory, lines_max*sizeof(vertex));
  arena elem_buffer_lines = subarena_init(&memory, lines_max*sizeof(u32));
  // Text (chars) buffer
  u32 text_vert_count = 6000;
  // 6000 text verts = 1000 quads
  arena text_buffer = text_buffer_init(&memory, text_vert_count);

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
  // Initialize render buffers
  render_buffer lines_gpu = render_buffer_init(nullptr, lines_max);
  render_buffer_attribute(lines_gpu, 0, 3, sizeof(fvec3), 0);
  u32 lines_program = render_program_init( &scratch, "shaders\\lines.vert", "shaders\\lines.frag");
  render_buffer text_gpu_buffer = text_gpu_init(text_vert_count);


  // Load application assets
  const char *font_file = "C:\\WINDOWS\\Fonts\\arial.ttf";
  // Create the char atlas bitmap image
  u32 text_texture_id = text_init( &scratch, font_file );
  u32 text_shader = render_program_init( &scratch, "shaders\\text.vert", "shaders\\text.frag");
  // You'll have to bind the texture each time you want to use this.
  const char *text_uniform = "texture_image";
  bool use_ndc = false;
  f32 text_scale = 0.0f;
  // Are the coordinates in screen space of NDC
  if (use_ndc)
  {
    text_scale = 2.0f / window.height; // NDC
  }
  else
  {
    text_scale = 1.0f; // screen space
  }

  // Set up instanced cube rendering for grid.
  render_buffer instance_buffer = instance_setup(&memory);
  u32 instance_program = render_program_init( &scratch, "shaders\\instance.vert", "shaders\\instance.frag");

  // Read in model data
  mesh teapot_model = model_load_obj("assets\\teapot.obj", &vert_buffer_lines, &elem_buffer_lines);
  fvec3 teapot_centroid = model_centroid(teapot_model);
  size_t teapot_buffer_size = sizeof(teapot_model.vertices[0]) * teapot_model.vert_count;
  render_buffer teapot_buffer = render_buffer_init((void*)teapot_model.vertices, teapot_buffer_size);
  render_buffer_attribute(
    teapot_buffer,
    0, 
    ARRAY_COUNT(teapot_model.vertices[0].pos.array),
    ARRAY_COUNT(teapot_model.vertices[0].pos.array) * sizeof(teapot_model.vertices[0].pos.array[0]),
    (void *)0
  );
  u32 teapot_program = render_program_init(&scratch, "shaders\\points.vert", "shaders\\points.frag");
  // Get bounding box
  mesh bbox = model_bbox_add(&vert_buffer_lines, &elem_buffer_lines, teapot_model);
  size_t bbox_ebo_size = sizeof(bbox.indices[0]) * bbox.index_count;
  render_buffer_push(lines_gpu, (void*)bbox.vertices, 0, 8 * sizeof(vertex));
  render_buffer_elements_init(&lines_gpu, bbox.indices, bbox_ebo_size);

  // Set up the angular speed variable for the rotation
  f32 angle_velocity = PI/4.0f;
  f32 angle = 0.0f;
  // How far is the camera from the model?
  f32 cam_distance =  2.5 * fvec3_max_elem(model_max(teapot_model));

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

    // Free resources
    arena_free_all(&scratch);
    arena_free_all( &text_buffer );

    // Check window dimensions
    window_size_get(&window);
    // Set render window dimensions (for now the whole canvas)
    renderer.width = window.width;
    renderer.height = window.height;
    // Initialize frame
    frame_init(&renderer);

    // Draw the UI
    // Bind the character atlas
    i32 text_slot = 0;
    texture_bind(text_slot, text_texture_id);
    // Set the uniform variables
    uniform_set_i32(text_shader, text_uniform, text_slot);
    glm::mat4 ortho = glm::ortho(0.0f, renderer.width, 0.0f, renderer.height, -1.0f, 1.0f);
    uniform_set_mat4(text_shader, "projection", &ortho[0][0]);
    // Add text
    // glm::vec3 tpos = glm::vec3(renderer.width * 0.5f, renderer.height * 0.5f, 0.0f);
    glm::vec3 tpos = glm::vec3(0.0f, 0.0f, 0.0f);
    // Add text data to gpu buffer
    text_add(&text_buffer, "Hello!", 6, window.height, tpos, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}, text_scale);
    render_buffer_push(text_gpu_buffer, text_buffer.buffer, 0, text_buffer.offset_new);
    // Draw text
    u32 text_vert_count = text_count_get(&text_buffer);
    draw_text(text_gpu_buffer, text_shader, text_vert_count);

    glm::mat4 identity = glm::mat4(1.0f);
    // Create view and projection matrix
    angle += angle_velocity * app_clock.delta; // rad += (rad/s)*s
    // wrap angle so it doesn't explode
    if (angle > 2.0*PI) angle -= 2.0*PI;
    f32 fov_deg = 45.0f;            // pick your FOV
    f32 aspect  = window.width / (window.height + 0.000001); // keep updated on resize
    glm::mat4 perspective_model = glm::mat4(1.0f);
    glm::vec3 rotation_axis_norm = glm::vec3(0,1,0);
    perspective_model = glm::rotate(perspective_model, angle, rotation_axis_norm);
    // look at
    glm::vec3 camera_pos    = glm::vec3(0, teapot_centroid.y, cam_distance);
    glm::vec3 camera_target = glm::vec3(teapot_centroid.x,teapot_centroid.y,teapot_centroid.z);
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

    // Draw the model's bounding box
    uniform_set_mat4(lines_program, "view_projection", &mvp[0][0]);
    i64 offset = (address)bbox.indices - (address) elem_buffer_lines.buffer;
    draw_lines_elements(lines_gpu, lines_program, bbox.index_count, (void*)offset);

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
