#include "core.cpp"
#include "linalg.cpp"
#include "platform.h"
#include "render.h"
#include "app_data.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Globals
global bool is_running;


#ifdef _OPENGL
#include "render_opengl.cpp"
#endif

// Application layers
#include "text.cpp"
#include "primitives.cpp"
// #include "data3d.cpp"

// TODO: Why does app crash when I share it with discord?

struct camera
{
  glm::mat4 view_inv;            // 64 bytes
  glm::mat4 proj_inv;            // 64 bytes
  glm::mat4 wrld_inv;            // 64 bytes - inverse rotation for volume
  glm::vec3 pos;                 // 12 bytes
  f32 _padding;                  // 4 bytes → pad to 16-byte multiple
};


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


inline u32 index3d( u32 x, u32 y, u32 z, u32 width, u32 height )
{
  return x + y * width + z * width * height;
}

texture_ptr image3d_create( arena *a )
{
  // Create a higher resolution volume with gradient density
  i32 resolution = 32;  // 32x32x32 = 32768 voxels
  u32 total_voxels = resolution * resolution * resolution;
  u8 *voxel_data = arena_push_array(a, total_voxels, u8);

  // Create a radial gradient sphere
  // Density = 1.0 at center, fading to 0.0 at edges
  f32 center = resolution / 2.0f;
  f32 max_radius = resolution / 2.0f;

  for (i32 z = max_radius; z < resolution; ++z)
  {
    for (i32 y = 0; y < resolution; ++y)
    {
      for (i32 x = 0; x < resolution; ++x)
      {
        // Calculate distance from center
        f32 dx = (f32)x - center;
        f32 dy = (f32)y - center;
        f32 dz = (f32)z - center;
        f32 distance = sqrtf(dx*dx + dy*dy + dz*dz);

        // Normalize distance to [0, 1] range
        f32 normalized_dist = distance / max_radius;

        // Invert so center is high density
        f32 density = 1.0f - normalized_dist;

        // Clamp to [0, 1]
        if (density < 0.0f) density = 0.0f;
        if (density > 1.0f) density = 1.0f;

        // Convert to u8 [0, 255]
        u8 value = (u8)(density * 255.0f);

        // Store in volume
        u32 index = index3d(x, y, z, resolution, resolution);
        voxel_data[index] = value;
      }
    }
  }

  // Add colored corner markers (different densities = different colors)
  // Each corner is a 3x3x3 block for better visibility
  // Heat map: 0-63=Blue, 64-127=Cyan, 128-191=Yellow, 192-255=White

  u8 corner_colors[8] = {
    32,   // Corner 0: Blue (low density)
    64,   // Corner 1: Blue-Cyan transition
    96,   // Corner 2: Cyan
    128,  // Corner 3: Cyan-Yellow transition
    160,  // Corner 4: Yellow
    192,  // Corner 5: Yellow-White transition
    224,  // Corner 6: Near white
    255   // Corner 7: White (max density)
  };

  i32 corner_positions[8][3] = {
    {0, 0, 0},
    {resolution-1, 0, 0},
    {0, resolution-1, 0},
    {resolution-1, resolution-1, 0},
    {0, 0, resolution-1},
    {resolution-1, 0, resolution-1},
    {0, resolution-1, resolution-1},
    {resolution-1, resolution-1, resolution-1}
  };

  // Draw 3x3x3 blocks at each corner
  for (i32 corner = 0; corner < 8; ++corner)
  {
    i32 cx = corner_positions[corner][0];
    i32 cy = corner_positions[corner][1];
    i32 cz = corner_positions[corner][2];

    for (i32 dz = -1; dz <= 1; ++dz)
    {
      for (i32 dy = -1; dy <= 1; ++dy)
      {
        for (i32 dx = -1; dx <= 1; ++dx)
        {
          i32 x = cx + dx;
          i32 y = cy + dy;
          i32 z = cz + dz;

          // Clamp to volume bounds
          if (x >= 0 && x < resolution &&
              y >= 0 && y < resolution &&
              z >= 0 && z < resolution)
          {
            voxel_data[index3d(x, y, z, resolution, resolution)] = corner_colors[corner];
          }
        }
      }
    }
  }

  // Upload to GPU
  texture_ptr voxel_texture = texture3d_init( a, voxel_data, resolution, resolution, resolution );
  return voxel_texture;
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

  // Prepare buffers
  u32 VERTEX_MAX = 100000;
  arena vbuffer_cpu = subarena_init( &memory, sizeof(vertex1)*VERTEX_MAX );
  arena ebuffer_cpu = subarena_init( &memory, sizeof(u32)*VERTEX_MAX );
  rbuffer_ptr vbuffer_gpu = render_buffer_dynamic_init(
    &memory,
    VERTS,
    vbuffer_cpu.buffer,
    sizeof(vertex1),
    vbuffer_cpu.length
  );
  rbuffer_ptr ebuffer_gpu = render_buffer_dynamic_init(
    &memory,
    ELEMS,
    ebuffer_cpu.buffer,
    sizeof(u32),
    ebuffer_cpu.length
  );
  primitive_box2d( &vbuffer_cpu, &ebuffer_cpu );
  // TODO: Update whole buffer or just what you need with offset_new?
  render_buffer_update( vbuffer_gpu, vbuffer_cpu.buffer, vbuffer_cpu.length );
  render_buffer_update( ebuffer_gpu, ebuffer_cpu.buffer, ebuffer_cpu.length );
  // Load shaders
  shaders_ptr tri_prog = shader_init(&memory);
  shader_load(tri_prog, VERTEX, "shaders/test.hlsl", "VSMain", "vs_5_0");
  shader_load(tri_prog, PIXEL,  "shaders/test.hlsl", "PSMain", "ps_5_0");

  // Camera stuff
  camera cam = {};
  cam.pos = glm::vec3(0.0f, 0.0f, -2.0f);

  // Create view matrix (lookAt camera)
  glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, 0.0f);  // Looking at origin
  glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);      // Y-up
  glm::mat4 view = glm::lookAt(cam.pos, camera_target, camera_up);

  // Create projection matrix (perspective)
  f32 fov_deg = glm::radians( 45.0f );
  f32 aspect = (f32)window.width / (f32)window.height;
  f32 znear = 0.1f;
  f32 zfar = 100.0f;
  glm::mat4 projection = glm::perspective(fov_deg, aspect, znear, zfar);

  // Compute inverses for raymarching shader
  cam.view_inv = glm::inverse(view);
  cam.proj_inv = glm::inverse(projection);

  // Initialize volume rotation (will be updated in render loop)
  cam.wrld_inv = glm::mat4(1.0f); // Identity for now

  // Upload to GPU
  rbuffer_ptr camera_gpu = render_buffer_constant_init( &memory, sizeof(camera) );
  render_buffer_update( camera_gpu, &cam, sizeof(camera) );
  // Bind camera constant buffer to pixel shader
  render_constant_set( camera_gpu, 0 );

  // Create view projection matrix
  // No projection (identity) 
  // glm::mat4 view_projection = glm::mat4(1.0f);
  // Orthographic projection
  // glm::mat4 view_projection = glm::ortho( 0.0f, window.width, 0.0f, window.height, znear, zfar );
  // glm::mat4 view_projection = glm::ortho(-5.0f, 5.0f,-5.0f, 5.0f, znear, zfar );
  // Perspective projection
  /*
  f32 znear = 0.1f;
  f32 zfar = 100.0f;
  f32 aspect  = window.width / (window.height + 0.000001); // keep updated on resize
  glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, znear, zfar);
  glm::mat4 view = glm::lookAt(glm::vec3(0,0,-5), glm::vec3(0,0,0), glm::vec3(0,1,0));
  glm::mat4 view_projection = projection * view;
  */

  // rbuffer_ptr viewproj_mat = render_buffer_constant_init( &memory, sizeof(view_projection) );
  // render_buffer_update( viewproj_mat, &view_projection, sizeof(view_projection) );
  // render_constant_set( viewproj_mat, 1 );

  // Create a 3D texture
  texture_ptr voxel_texture = image3d_create( &memory );

  // Create transfer function (1D texture for color mapping)
  // Heat map: Black → Blue → Cyan → Green → Yellow → Red → White
  i32 tf_size = 256;
  u8 *tf_data = arena_push_array(&memory, tf_size * 4, u8); // RGBA
  for (i32 i = 0; i < tf_size; ++i)
  {
    f32 t = (f32)i / (f32)(tf_size - 1); // 0.0 to 1.0
    u8 r, g, b, a;

    if (t < 0.25f) {
      // Black → Blue
      f32 s = t / 0.25f;
      r = 0;
      g = 0;
      b = (u8)(s * 255);
      a = (u8)(s * 255); // Fade in opacity
    } else if (t < 0.5f) {
      // Blue → Cyan
      f32 s = (t - 0.25f) / 0.25f;
      r = 0;
      g = (u8)(s * 255);
      b = 255;
      a = 255;
    } else if (t < 0.75f) {
      // Cyan → Yellow
      f32 s = (t - 0.5f) / 0.25f;
      r = (u8)(s * 255);
      g = 255;
      b = (u8)((1.0f - s) * 255);
      a = 255;
    } else {
      // Yellow → White
      f32 s = (t - 0.75f) / 0.25f;
      r = 255;
      g = 255;
      b = (u8)(s * 255);
      a = 255;
    }

    tf_data[i * 4 + 0] = r;
    tf_data[i * 4 + 1] = g;
    tf_data[i * 4 + 2] = b;
    tf_data[i * 4 + 3] = a;
  }

  texture_ptr transfer_function = texture1d_init(&memory, tf_data, tf_size);

  // Load raymarching shaders
  shaders_ptr raytrace_prog = shader_init(&memory);
  shader_load(raytrace_prog, VERTEX, "shaders/raymarching.hlsl", "VSMain", "vs_5_0");
  shader_load(raytrace_prog, PIXEL,  "shaders/raymarching.hlsl", "PSMain", "ps_5_0");

  // Read in a texture
  // const char* filename = "G:/pacbird/.assets/checker-gray5.png";
  // i32 test = platform_file_exists(filename);
  // i32 x, y, n;
  // i32 components_per_pixel = 4; // Force RGBA
  // unsigned char *data = stbi_load(filename, &x, &y, &n, components_per_pixel);
  // texture_ptr tri_texture = texture2d_init(&memory, data, x, y, components_per_pixel);

  // Initialize Text
  u32 text_vert_count = 6000;
  arena tbuffer_cpu = text_buffer_init(&memory, text_vert_count);
  const char *font_file = "C:/MyFonts/Source_Code_Pro/static/SourceCodePro-Regular.ttf";
  texture_ptr text_texture = text_init( &memory, font_file );
  // render_text_init(&memory);

  rbuffer_ptr tbuffer_gpu = render_buffer_dynamic_init(
    &memory,
    VERTS,
    tbuffer_cpu.buffer,
    sizeof(f32) * 9,
    tbuffer_cpu.length
  );

  shaders_ptr text_shaders = shader_init(&memory);
  shader_load(text_shaders, VERTEX, "shaders/text.hlsl", "VSMain", "vs_5_0");
  shader_load(text_shaders, PIXEL,  "shaders/text.hlsl", "PSMain", "ps_5_0");

  // Adding text.
  bool use_ndc = true;
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
  glm::vec3 tpos = glm::vec3(0.0f, 0.0f, 0.0f);
  bool is_ccw = false;
  text_add(&tbuffer_cpu, "TEXT!", 5, window.height, tpos, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}, text_scale);
  render_buffer_update(tbuffer_gpu, tbuffer_cpu.buffer, tbuffer_cpu.offset_new);

  // Rotation 
  f32 angle = 0.0f;
  f32 angle_velocity = PI/4.0f;

  // Initialize clock
  f64 fps_target = 60; // The amount of frames presented in a second.
  clock timer = platform_clock_init(fps_target);

  // Application start
  platform_window_show();
  platform_clock_reset(&timer);
  while (platform_is_running())
  {
    platform_message_process(&window);
    platform_clock_update(&timer);
    // printf("Delta: %.8f\n", timer.delta);

    arena_free_all( &scratch );

    frame_init();

    // Update volume rotation
    angle += angle_velocity * timer.delta; // rad += (rad/s)*s
    // wrap angle so it doesn't explode
    if (angle > 2.0*PI) angle -= 2.0*PI;

    glm::vec3 rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f); // Y-axis
    glm::mat4 volume_rotation = glm::rotate(glm::mat4(1.0f), angle, rotation_axis);
    cam.wrld_inv = glm::inverse(volume_rotation);

    // Update camera constant buffer with new rotation
    render_buffer_update(camera_gpu, &cam, sizeof(camera));

    // This adds rotation to the view/proj matrix
    /*
    // Update transforms
    angle += angle_velocity * timer.delta; // rad += (rad/s)*s
    // wrap angle so it doesn't explode
    if (angle > 2.0*PI) angle -= 2.0*PI;
    glm::mat4 world = glm::mat4(1.0f);
    glm::vec3 rotation_axis_norm = glm::vec3(0,1,0);
    // world = glm::rotate(world, angle, rotation_axis_norm);

    // I only have 1 object atm.
    glm::mat4 temp = view_projection * world;
    render_buffer_update( viewproj_mat, &temp, sizeof(temp) );
    */

    // Bind textures for raymarching
    texture_bind(voxel_texture, 0);
    texture_bind(transfer_function, 1);
    u32 elem_count = ebuffer_cpu.offset_new / sizeof(u32);
    // render_draw_elems( vbuffer_gpu, ebuffer_gpu, tri_prog, elem_count, 0, 0);
    render_draw_elems( vbuffer_gpu, ebuffer_gpu, raytrace_prog, elem_count, 0, 0);

    // Draw text
    texture_bind(text_texture, 0);
    u32 text_vert_count = text_count(&tbuffer_cpu);
    render_draw(tbuffer_gpu, text_shaders, text_vert_count);

    frame_render();
  }
  render_buffer_close( vbuffer_gpu );
  render_buffer_close( ebuffer_gpu );
  render_buffer_close( camera_gpu );
  render_buffer_close( tbuffer_gpu );
  texture_close( transfer_function );
  texture_close( voxel_texture );
  texture_close( text_texture );
  shader_close( tri_prog );
  shader_close( raytrace_prog );
  shader_close( text_shaders );
  render_close();
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
