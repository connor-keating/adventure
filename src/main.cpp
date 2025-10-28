#include "core.cpp"
#include "linalg.cpp"
#include "platform.h"


// Globals
global bool is_running;


#ifdef _DX11
#include "render_dx11.cpp"
#elif _OPENGL
#include "render_opengl.cpp"
#endif

// Application layers
// #include "text.cpp"
// #include "data3d.cpp"

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
  size_t memory_size = (size_t) Gigabytes(5);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena memory = arena_init(raw_memory, memory_size);

  // Init CPU buffers
  // Scratch arena that can be freed frequently.
  u32 scratch_max = Gigabytes(3);
  arena scratch = subarena_init(&memory, scratch_max);
  // Start the platform layer
  platform_init(&memory);

  // Create a window for the application
  platform_window window = platform_window_init();

  // Initialize renderer
  render_init(&memory);

  // Free scratch
  arena_free_all(&scratch);
  platform_window_show();
  while (platform_is_running())
  {
    platform_message_process(&window);

    arena_free_all( &scratch );

    frame_init();

    frame_render();
  }
  return 0;
}

#if 0
int main(int argc, char **argv)
{
  // Allocate all program memory upfront.
#if _DEBUG
  void *memory_base = (void*)Terabytes(2);
#else
  void *memory_base = 0;
#endif
  size_t memory_size = (size_t) Gigabytes(5);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena memory = arena_init(raw_memory, memory_size);

  // Init CPU buffers
  // Scratch arena that can be freed frequently.
  u32 scratch_max = Gigabytes(3);
  arena scratch = subarena_init(&memory, scratch_max);
  // Render buffer that contains line data
  u32 lines_max = 1000000;
  arena vert_buffer_lines = subarena_init(&memory, lines_max*sizeof(vertex));
  arena elem_buffer_lines = subarena_init(&memory, lines_max*sizeof(u32));
  // Text (chars) buffer
  u32 text_vert_count = 6000;
  // 6000 text verts = 1000 quads
  arena text_buffer = text_buffer_init(&memory, text_vert_count);

  // Start the platform layer
  platform_init(&memory);
  // Create a window for the application
  platform_window window = platform_window_init();

  // Application clock
  f64 fps_target = 60;                // The amount of frames presented in a second.
  clock app_clock = platform_clock_init(fps_target);

  // Initialize input map
  i32           input_map[ACTION_COUNT];
  control_state input_state[ACTION_COUNT];

  // Set the bindings
  // platform_control_set(input_map);

  // Initialize renderer
  render_state renderer = render_init(&window);
  // Initialize render buffers
  size_t lines_vbo_size = lines_max * sizeof(vertex);
  render_buffer lines_gpu = render_buffer_init(nullptr, lines_vbo_size);
  render_buffer_attribute(lines_gpu, 0, 3, sizeof(fvec3), 0);
  render_buffer_elements_init(&lines_gpu, nullptr, lines_max*sizeof(u32));
  u32 lines_program = render_program_init( &scratch, "shaders\\lines.vert", "shaders\\lines.frag");
  render_buffer text_gpu_buffer = text_gpu_init(text_vert_count);


  // Initialize debugging stuff
  render_point_size_set(10.0f);
  vertex origin_vert = {};
  origin_vert.pos = fvec3{ {0, 0, 0} };
  render_buffer origin_gpu = render_buffer_init(&origin_vert.pos, sizeof(origin_vert));
  render_buffer_attribute(origin_gpu, 0, 3, sizeof(fvec3), 0);
  u32 origin_program = render_program_init( &scratch, "shaders\\points.vert",  "shaders\\points.frag");


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

  // Read in model data
  mesh object_model = model_load_obj("assets\\bunny.obj", &vert_buffer_lines, &elem_buffer_lines);
  size_t object_buffer_size = sizeof(object_model.vertices[0]) * object_model.vert_count;
  render_buffer_push(lines_gpu, (void*)object_model.vertices, 0, object_buffer_size);
  i64 object_starting_byte = model_starting_offset(&elem_buffer_lines, object_model);
  i64 object_bytes = object_model.index_count * sizeof(u32);
  render_buffer_elements_push(lines_gpu, object_model.indices, object_starting_byte, object_bytes);

  // Get bounding box
  // mesh bbox = model_bbox_add(&vert_buffer_lines, &elem_buffer_lines, object_model);

  // Voxelize the model
  i32 voxel_count = 50;
  // TODO: Where should the memory of the grid be stored
  // voxel_grid grid = model_voxelize2(object_model, voxel_count, &vert_buffer_lines, &elem_buffer_lines, &memory);
  voxel_grid grid = model_voxelize_solid(object_model, voxel_count, &vert_buffer_lines, &elem_buffer_lines, &memory);
  mesh bbox = bbox_create(grid.min, grid.max, &vert_buffer_lines, &elem_buffer_lines);
  // Update model
  render_buffer_push(lines_gpu, (void*)object_model.vertices, 0, object_buffer_size);
  // Add bbox to renderer
  size_t bbox_vbo_size = sizeof(bbox.vertices[0]) * bbox.vert_count;
  render_buffer_push(lines_gpu, (void*)bbox.vertices, object_buffer_size, bbox_vbo_size);
  i64 bbox_starting_byte = model_starting_offset(&elem_buffer_lines, bbox);
  i64 bbox_starting_index = bbox_starting_byte / sizeof(bbox.indices[0]);
  i64 bbox_bytes   = bbox.index_count * sizeof(u32);
  for (int i = 0; i < bbox.index_count; ++i)
  {
    bbox.indices[i] += bbox_starting_index;
  }
  render_buffer_elements_push(lines_gpu, bbox.indices, bbox_starting_byte, bbox_bytes);

  // Free scratch
  arena_free_all(&scratch);

  // Set up instanced cube rendering for grid.
  mesh cube = primitive_cube(&scratch);
  render_buffer instance_buffer = render_buffer_init((void*)cube.vertices, cube.vert_count*sizeof(vertex));
  render_buffer_attribute(instance_buffer, 0, 3, 3*sizeof(f32), (void*)0);
  render_buffer_elements_init(&instance_buffer, cube.indices, cube.index_count*sizeof(u32));
  u32 instance_program = render_program_init( &scratch, "shaders\\instance.vert", "shaders\\instance.frag");
  fvec3 grid_shape = fvec3_uniform(voxel_count);
  voxel_grid_init(&scratch, grid_shape);

  // I want the grid to be controlled as a 3D texture.
  u32 grid_element_count = grid_shape.x * grid_shape.y * grid_shape.z;
  // u8 *grid_data = arena_push_array(&scratch, grid_element_count, u8);
  // memset(grid_data, 255, grid_element_count * sizeof(u8));
  for (size_t i = 0; i < grid_element_count; ++i) grid.contents[i] = grid.contents[i] * 255;

  u32 grid_texture_id = texture3d_init(grid.contents, grid_shape);
  // TODO: Renderer needs a better way of handling texture slots
  i32 grid_texture_slot = 1;

  // Set up the angular speed variable for the rotation
  bool rotation_on = false;
  f32 angle_velocity = PI/4.0f;
  f32 angle = 0.0f;
  // How far is the camera from the model?
  f32 cam_distance = 3*grid.max.z;
  // Instance shader toggle
  bool toggle = 0;

  render_vsync(0);
  // Show window
  platform_window_show();
  while (platform_is_running())
  {
    // Frame start
    platform_clock_update(&app_clock);
    platform_message_process(&window);

    // Free resources
    arena_free_all(&scratch);
    arena_free_all( &text_buffer );

    // Check window dimensions
    platform_window_size(&window);
    // Set render window dimensions (for now the whole canvas)
    renderer.width = window.width;
    renderer.height = window.height;

    // Determine user input
    if (input_state[ACTION1] == CONTROL_DOWN)
    {
      toggle = !toggle;
    }

    // Initialize frame
    frame_init(&renderer);

    // Calculate camera settings
    // Create view and projection matrix
    f32 fov_deg = 45.0f;            // pick your FOV
    f32 aspect  = window.width / (window.height + 0.000001); // keep updated on resize
    // I want to start and stop rotation based on user input.
    if (toggle)
    {
      angle += angle_velocity * app_clock.delta; // rad += (rad/s)*s
      // wrap angle so it doesn't explode
      if (angle > 2.0*PI) angle -= 2.0*PI;
    }
    // look at
    fvec3 bbox_max = model_max(bbox);
    fvec3 target = model_centroid(bbox);
    glm::vec3 target_gpu = glm::vec3(target.x, target.y, target.z);
    glm::vec3 camera_pos    = glm::vec3(target.x, target.y, cam_distance);
    glm::vec3 camera_target = target_gpu;
    glm::vec3 camera_up     = glm::vec3(0,1,0);
    glm::mat4 view = glm::lookAt(camera_pos, camera_target, camera_up);
    // perspective
    f32 fov_rad = glm::radians(fov_deg);
    f32 znear = 0.1f;
    f32 zfar = 100.0f;
    glm::mat4 perspective_projection = glm::perspective(fov_rad, aspect, znear, zfar);
    glm::mat4 view_projection = perspective_projection * view;

    // Draw origin
    uniform_set_mat4(origin_program, "view_projection", &view_projection[0][0]);
    draw_points( origin_gpu,  origin_program, 1);

    // Draw the UI
    // Bind the character atlas
    i32 text_slot = 0;
    texture2d_bind(text_slot, text_texture_id);
    // Set the uniform variables
    uniform_set_i32(text_shader, text_uniform, text_slot);
    glm::mat4 ortho = glm::ortho(0.0f, renderer.width, 0.0f, renderer.height, -1.0f, 1.0f);
    uniform_set_mat4(text_shader, "projection", &ortho[0][0]);
    // Add text
    // glm::vec3 tpos = glm::vec3(renderer.width * 0.5f, renderer.height * 0.5f, 0.0f);
    glm::vec3 tpos = glm::vec3(0.0f, 0.0f, 0.0f);
    // Add text data to gpu buffer
    text_add(&text_buffer, "TEXT!", 5, window.height, tpos, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}, text_scale);
    render_buffer_push(text_gpu_buffer, text_buffer.buffer, 0, text_buffer.offset_new);
    // Draw text
    u32 text_vert_count = text_count_get(&text_buffer);
    draw_text(text_gpu_buffer, text_shader, text_vert_count);

    glm::mat4 object_transform = glm::mat4(1.0f);
    glm::vec3 rotation_axis_norm = glm::vec3(0,1,0);
    object_transform = glm::translate(object_transform, target_gpu);
    object_transform = glm::rotate(object_transform, angle, rotation_axis_norm);
    object_transform = glm::translate(object_transform, -target_gpu);
    glm::mat4 object_mvp = view_projection * object_transform;
    // Draw the model
    uniform_set_mat4(lines_program, "view_projection", &object_mvp[0][0]);
    i64 object_offset = model_starting_offset(&elem_buffer_lines, object_model);
    draw_wireframe_elements(lines_gpu, lines_program, object_model.index_count, (void*)object_offset);

    // Draw the model's bounding box 
    glm::mat4 perspective_model = glm::mat4(1.0f);
    perspective_model = glm::translate(perspective_model, target_gpu);
    perspective_model = glm::rotate(perspective_model, angle, rotation_axis_norm);
    perspective_model = glm::translate(perspective_model, -target_gpu);
    glm::mat4 mvp = view_projection * perspective_model;
    uniform_set_mat4(lines_program, "view_projection", &mvp[0][0]);
    i64 bbox_index_offset  = model_starting_offset(&elem_buffer_lines, bbox);
    // draw_lines_elements(lines_gpu, lines_program, bbox.index_count, (void*)bbox_index_offset);

    // You could draw the whole buffer at once if you wanted to...
    /*
    // Draw the line buffer
    i64 elem_count = elem_buffer_lines.offset_new / sizeof(u32);
    draw_lines_elements(lines_gpu, lines_program, elem_count, 0);
    */

    // Set color for instance grid
    fvec3 color = {};
    color.array[0] = 0.0f;
    color.array[1] = 1.0f;
    color.array[2] = 0.0f;
    uniform_set_vec3(instance_program, "mycolor", color);

    // Draw instance cube
    glm::mat4 grid_model = glm::mat4(1.0f);
    glm::vec3 grid_center = glm::vec3(target.x, target.y, target.z);
    fvec3 bbox_min = model_min(bbox);
    // fvec3 bbox_max = model_max(bbox);
    fvec3 temp = fvec3_scale(fvec3_sub(bbox_max, bbox_min), 0.5f);
    glm::vec3 grid_shape2 = glm::vec3(temp.x, temp.y, temp.z);
    // Normally it goes t * r * s, but here rotation should be last so it is rotated about the model center.
    grid_model = glm::translate(grid_model, grid_center);
    grid_model = glm::rotate(grid_model, angle, rotation_axis_norm);
    grid_model = glm::scale(grid_model, grid_shape2);
    glm::mat4 grid_mvp = view_projection * grid_model;
    uniform_set_mat4(instance_program, "view_projection", &grid_mvp[0][0]);
    // Grid Texture
    texture3d_bind(grid_texture_slot, grid_texture_id);
    uniform_set_i32(instance_program, "voxels", grid_texture_slot);
    uniform_set_vec3(instance_program, "dimension", grid_shape);
    draw_lines_instanced(instance_buffer, instance_program, (grid_shape.x * grid_shape.y * grid_shape.z));

    // Finalize and draw frame
    frame_render();
    // Reset input after processing everything
    input_reset(input_state);
  }

  // close and release all existing COM objects
  // program_close(&prog);
  // program_close(&prog2);

  return 0;
}
#endif
