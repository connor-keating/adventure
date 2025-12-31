#include "application.h"
#include "linalg.cpp"

#include "core.h"
#include "input.h"
#include "platform.h"
#include "render.h"
#include "primitives.cpp"

#include "app_data.h"


#define FPS_TARGET        60
#define MAX_COUNT_UI      1000
#define MAX_COUNT_VERTEX  100000
#define MAX_COUNT_RBUFFER 100
#define MAX_COUNT_SHADERS 100

struct camera
{
  glm::mat4 view_inv;            // 64 bytes
  glm::mat4 proj_inv;            // 64 bytes
  glm::mat4 wrld_inv;            // 64 bytes - inverse rotation for volume
  glm::vec3 pos;                 // 12 bytes
  f32 _padding;                  // 4 bytes → pad to 16-byte multiple
};

struct appstate
{
  bool            is_running;
  platform_window window;
  clock           timer;
  input_state     inputs[KEY_COUNT];
  arena           vbuffer_cpu; // Vertex buffer
  arena           ebuffer_cpu; // Element buffer
  rbuffer        *vbuffer_gpu;
  rbuffer        *ebuffer_gpu;
  camera          cam;
  rbuffer        *camera_ray;
  rbuffer        *ui_matrix;
  u64             shader[MAX_COUNT_SHADERS];
};

global appstate *state;


internal inline u32 index3d( u32 x, u32 y, u32 z, u32 width, u32 height )
{
return x + y * width + z * width * height;
}

internal texture* image3d_create( arena *a )
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
  texture* voxel_texture = texture3d_init( a, voxel_data, resolution, resolution, resolution );
  return voxel_texture;
}


bool app_is_running()
{
  state->is_running = platform_is_running();
  return state->is_running;
}

void app_init( arena *memory )
{
  // Create internal global state
  state = arena_push_struct( memory, appstate );
  state->is_running = false;
  // Start the platform layer
  platform_init( memory );
  // Create a window for the application
  state->window = platform_window_init();
  // Initialize renderer
  render_init( memory );
  render_data_init( memory, MAX_COUNT_SHADERS );
  state->vbuffer_cpu = subarena_init( memory, MAX_COUNT_VERTEX * sizeof(vertex1) );
  state->ebuffer_cpu = subarena_init( memory, MAX_COUNT_VERTEX * sizeof(u32) );
  state->vbuffer_gpu = rbuffer_dynamic_init(
    memory,
    BUFF_VERTS,
    state->vbuffer_cpu.buffer,
    sizeof(vertex1),
    state->vbuffer_cpu.length
  );
  state->ebuffer_gpu = rbuffer_dynamic_init(
    memory,
    BUFF_ELEMS,
    state->ebuffer_cpu.buffer,
    sizeof(u32),
    state->ebuffer_cpu.length
  );
  // Load UI shaders
  state->shader[0] = shader_init( memory );
  shader_load( state->shader[0], VERTEX, "shaders/ui.hlsl", "VSMain", "vs_5_0");
  shader_load( state->shader[0], PIXEL,  "shaders/ui.hlsl", "PSMain", "ps_5_0");
  glm::mat4 identity = glm::mat4(1.0f);
  state->ui_matrix = rbuffer_dynamic_init( memory, BUFF_CONST, &identity, 0, sizeof(identity) );
  // Load raymarching shaders
  state->shader[1] = shader_init( memory );
  shader_load( state->shader[1], VERTEX, "shaders/raymarching.hlsl", "VSMain", "vs_5_0");
  shader_load( state->shader[1], PIXEL,  "shaders/raymarching.hlsl", "PSMain", "ps_5_0");
  // Constant buffer
  // Camera
  state->cam.pos = glm::vec3(0.0f, 0.0f, -2.0f);
  // Create view matrix (lookAt camera)
  glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, 0.0f);  // Looking at origin
  glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);      // Y-up
  glm::mat4 view = glm::lookAt(state->cam.pos, camera_target, camera_up);
  // Create projection matrix (perspective)
  f32 fov_deg = glm::radians( 45.0f );
  f32 aspect = (f32)state->window.width / (f32)state->window.height;
  f32 znear = 0.1f;
  f32 zfar = 100.0f;
  glm::mat4 projection = glm::perspective(fov_deg, aspect, znear, zfar);
  // Compute inverses for raymarching shader
  state->cam.view_inv = glm::inverse(view);
  state->cam.proj_inv = glm::inverse(projection);
  // Initialize volume rotation (will be updated in render loop)
  state->cam.wrld_inv = glm::mat4(1.0f); // Identity for now
  state->camera_ray = rbuffer_dynamic_init( memory, BUFF_CONST, &state->cam, 0, sizeof(camera) );
  rbuffer_update( state->camera_ray, &state->cam, sizeof(camera) );
  // Bind camera constant buffer to pixel shader
  render_constant_set( state->camera_ray, 0 );
  // Create 3D image texture for raymarching
  texture* voxel_texture = image3d_create( memory );
  texture_bind(voxel_texture, 0);
  // Create color transfer function for raymarching
  // Create transfer function (1D texture for color mapping)
  // Heat map: Black → Blue → Cyan → Green → Yellow → Red → White
  i32 tf_size = 256;
  u8 *tf_data = arena_push_array( memory, tf_size * 4, u8); // RGBA
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
  texture* transfer_function = texture1d_init( memory, tf_data, tf_size);
  texture_bind(transfer_function, 1);
  // Initialize the timer
  state->timer = platform_clock_init(FPS_TARGET);
  platform_clock_update(&state->timer);
  // Application startup complete, show the window.
  platform_window_show();
}

void app_update( arena *memory )
{
  // Empty message queue
  platform_message_process( &state->window, state->inputs );
  if ( state->inputs[KEY_ESCAPE] == INPUT_DOWN )
  {
    platform_window_close();
  }
  platform_clock_update(&state->timer);
  fvec4 frame_background = fvec4_init( 0.0f, 0.325f, 0.282f, 1.0f );
  frame_init( frame_background.array );
  // Reset render data buffers
  arena_free_all( &state->vbuffer_cpu );
  arena_free_all( &state->ebuffer_cpu );
  // Add raymarching quad to buffers
  model3d raybox2d = primitive_box2d( &state->vbuffer_cpu, &state->ebuffer_cpu, fvec4_uniform(0.0f) );
  model3d button_test = primitive_box2d( &state->vbuffer_cpu, &state->ebuffer_cpu, fvec4_init(1.0f, 0.0f, 0.0f, 1.0f) );
  // TODO: Update whole buffer or just what you need with offset_new?
  rbuffer_update( state->vbuffer_gpu, state->vbuffer_cpu.buffer, state->vbuffer_cpu.length );
  rbuffer_update( state->ebuffer_gpu, state->ebuffer_cpu.buffer, state->ebuffer_cpu.length );
  // Update volume rotation
  static f32 angle = 0.0f;
  angle += (PI/4.0f) * state->timer.delta; // rad += (rad/s)*s
  // wrap angle so it doesn't explode
  if (angle > 2.0*PI) angle -= 2.0*PI;
  glm::vec3 rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f); // Y-axis
  glm::mat4 volume_rotation = glm::rotate(glm::mat4(1.0f), angle, rotation_axis);
  state->cam.wrld_inv = glm::inverse(volume_rotation);
  render_constant_set( state->camera_ray, 0 );
  rbuffer_update( state->camera_ray, &state->cam, sizeof(camera));
  // Draw raymarched quad
  render_draw_elems( 
    state->vbuffer_gpu, 
    state->ebuffer_gpu, 
    state->shader[1],  // Shader ray ID
    raybox2d.count, 
    raybox2d.elem_start,
    raybox2d.vert_start
  );
  // Create UI elements
  glm::mat4 ui_world = glm::mat4(1.0f);
  ui_world = glm::translate( ui_world, glm::vec3( 0.5f, 0.0f, 0.0f ) );
  ui_world = glm::scale( ui_world, glm::vec3(0.2f, 0.2f, 0.0f) );
  rbuffer_update( state->ui_matrix, &ui_world, sizeof(ui_world) );
  render_constant_set( state->ui_matrix, 1 );
  render_draw_ui_elems( 
    state->vbuffer_gpu, 
    state->ebuffer_gpu, 
    state->shader[0], 
    button_test.count, 
    button_test.elem_start,
    button_test.vert_start
  );
  ui_world = glm::mat4(1.0f);
  ui_world = glm::translate( ui_world, glm::vec3( -0.5f, 0.0f, 0.0f ) );
  ui_world = glm::scale( ui_world, glm::vec3(0.2f, 0.2f, 0.0f) );
  rbuffer_update( state->ui_matrix, &ui_world, sizeof(ui_world) );
  render_draw_ui_elems( 
    state->vbuffer_gpu, 
    state->ebuffer_gpu, 
    state->shader[0], 
    button_test.count, 
    button_test.elem_start,
    button_test.vert_start
  );
  frame_render();
}

// An old main implementation of the raymarcher for reference. Delete this when you don't need it anymore.
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

  app_init( &memory );

  // Init CPU buffers
  // Scratch arena that can be freed frequently.
  u32 scratch_max = Gigabytes(3);
  arena scratch = subarena_init(&memory, scratch_max);

  // Initialize renderer
  render_init(&memory);

  // Prepare buffers
  u32 VERTEX_MAX = 100000;
  arena vbuffer_cpu = subarena_init( &memory, sizeof(vertex1)*VERTEX_MAX );
  arena ebuffer_cpu = subarena_init( &memory, sizeof(u32)*VERTEX_MAX );
  rbuffer* vbuffer_gpu = rbuffer_dynamic_init(
    &memory,
    BUFF_VERTS,
    vbuffer_cpu.buffer,
    sizeof(vertex1),
    vbuffer_cpu.length
  );
  rbuffer* ebuffer_gpu = rbuffer_dynamic_init(
    &memory,
    BUFF_ELEMS,
    ebuffer_cpu.buffer,
    sizeof(u32),
    ebuffer_cpu.length
  );
  primitive_box2d( &vbuffer_cpu, &ebuffer_cpu );
  // TODO: Update whole buffer or just what you need with offset_new?
  rbuffer_update( vbuffer_gpu, vbuffer_cpu.buffer, vbuffer_cpu.length );
  rbuffer_update( ebuffer_gpu, ebuffer_cpu.buffer, ebuffer_cpu.length );
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
  rbuffer* camera_gpu = rbuffer_dynamic_init( &memory, BUFF_CONST, &cam, 0, sizeof(camera) );
  rbuffer_update( camera_gpu, &cam, sizeof(camera) );
  // Bind camera constant buffer to pixel shader
  render_constant_set( camera_gpu, 0 );

  // Create view projection matrix
  // No projection (identity) 
  // glm::mat4 view_projection = glm::mat4(1.0f);
  // Orthographic projection
  // glm::mat4 view_projection = glm::ortho( 0.0f, window.width, 0.0f, window.height, znear, zfar );
  // Perspective projection
  /*
  f32 znear = 0.1f;
  f32 zfar = 100.0f;
  f32 aspect  = window.width / (window.height + 0.000001); // keep updated on resize
  glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, znear, zfar);
  glm::mat4 view = glm::lookAt(glm::vec3(0,0,-5), glm::vec3(0,0,0), glm::vec3(0,1,0));
  glm::mat4 view_projection = projection * view;
  */

  // rbuffer* viewproj_mat = rbuffer_dynamic_init( &memory, sizeof(view_projection) );
  // rbuffer_update( viewproj_mat, &view_projection, sizeof(view_projection) );
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
  // const char* texture_checkers = "G:/pacbird/.assets/checker-gray5.png";
  // texture_read( texture_checkers, &memory );

  // Initialize UI
  arena ui_vbuffer_cpu = subarena_init( &memory, sizeof(vertex1)*VERTEX_MAX );
  arena ui_ebuffer_cpu = subarena_init( &memory, sizeof(u32)*VERTEX_MAX );
  rbuffer* ui_vbuffer_gpu = rbuffer_dynamic_init(
    &memory,
    BUFF_VERTS,
    ui_vbuffer_cpu.buffer,
    sizeof(vertex1),
    ui_vbuffer_cpu.length
  );
  rbuffer* ui_ebuffer_gpu = rbuffer_dynamic_init(
    &memory,
    BUFF_ELEMS,
    ui_ebuffer_cpu.buffer,
    sizeof(u32),
    ui_ebuffer_cpu.length
  );
  primitive_box2d( &ui_vbuffer_cpu, &ui_ebuffer_cpu );
  rbuffer_update( ui_vbuffer_gpu, ui_vbuffer_cpu.buffer, ui_vbuffer_cpu.length );
  rbuffer_update( ui_ebuffer_gpu, ui_ebuffer_cpu.buffer, ui_ebuffer_cpu.length );
  // UI Shaders
  shaders_ptr ui_shaders = shader_init(&memory);
  shader_load(ui_shaders, VERTEX, "shaders/ui.hlsl", "VSMain", "vs_5_0");
  shader_load(ui_shaders, PIXEL,  "shaders/ui.hlsl", "PSMain", "ps_5_0");
  // UI ViewProjection
  glm::mat4 box_transform = glm::mat4(1.0f);
  box_transform = glm::translate(
    box_transform,
    glm::vec3( 300.0f, -100.0f, 0.0f )
  );
  box_transform = glm::scale(
    box_transform,
    glm::vec3(100.0f, 50.0f, 0.0f)
  );
  f32 xmax = window.width / 2.0f;
  f32 ymax = window.height / 2.0f;
  f32 xmin = -xmax;
  f32 ymin = -ymax;
  glm::mat4 ui_view_projection = glm::ortho(
    xmin, xmax,
    ymin, ymax,
    -1.0f,  1.0f 
  );
  ui_view_projection *= box_transform;
  rbuffer* ui_view_proj_gpu = rbuffer_dynamic_init( &memory, BUFF_CONST, &ui_view_projection, 0, sizeof(ui_view_projection) );
  rbuffer_update( ui_view_proj_gpu, &ui_view_projection, sizeof(ui_view_projection) );

  // Initialize Text
  u32 text_vert_count = 6000;
  arena tbuffer_cpu = text_buffer_init(&memory, text_vert_count);
  const char *font_file = "C:/MyFonts/Source_Code_Pro/static/SourceCodePro-Regular.ttf";
  texture_ptr text_texture = text_init( &memory, font_file );
  // render_text_init(&memory);
  rbuffer* tbuffer_gpu = rbuffer_dynamic_init(
    &memory,
    BUFF_VERTS,
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
  rbuffer_update(tbuffer_gpu, tbuffer_cpu.buffer, tbuffer_cpu.offset_new);

  // Rotation 
  f32 angle = 0.0f;
  f32 angle_velocity = PI/4.0f;

  // Initialize clock
  f64 fps_target = 60; // The amount of frames presented in a second.
  clock timer = platform_clock_init(fps_target);

  // Application start
  input_state inputs[KEY_COUNT];
  platform_window_show();
  platform_clock_reset(&timer);
  while (platform_is_running())
  {
    platform_message_process( &window, inputs );

    if (inputs[KEY_ESCAPE] == INPUT_DOWN)
    {
      platform_window_close();
    }
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
    render_constant_set( camera_gpu, 0 );
    rbuffer_update(camera_gpu, &cam, sizeof(camera));

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
    rbuffer_update( viewproj_mat, &temp, sizeof(temp) );
    */

    // Bind textures for raymarching
    texture_bind(voxel_texture, 0);
    texture_bind(transfer_function, 1);
    u32 elem_count = ebuffer_cpu.offset_new / sizeof(u32);
    // render_draw_elems( vbuffer_gpu, ebuffer_gpu, tri_prog, elem_count, 0, 0);
    render_draw_elems( vbuffer_gpu, ebuffer_gpu, raytrace_prog, elem_count, 0, 0);
    
    // Draw UI background stuff
    render_constant_set( ui_view_proj_gpu, 1 );
    rbuffer_update( ui_view_proj_gpu, &ui_view_projection, sizeof(ui_view_projection));
    u32 ui_elem_count = ui_ebuffer_cpu.offset_new / sizeof(u32);
    render_draw_ui_elems( ui_vbuffer_gpu, ui_ebuffer_gpu, ui_shaders, ui_elem_count, 0, 0);

    // Draw text
    texture_bind(text_texture, 0);
    u32 text_vert_count = text_count(&tbuffer_cpu);
    render_draw_ui(tbuffer_gpu, text_shaders, text_vert_count);

    frame_render();
  }
  rbuffer_close( vbuffer_gpu );
  rbuffer_close( ebuffer_gpu );
  rbuffer_close( ui_vbuffer_gpu );
  rbuffer_close( ui_ebuffer_gpu );
  rbuffer_close( ui_view_proj_gpu );
  rbuffer_close( camera_gpu );
  rbuffer_close( tbuffer_gpu );
  texture_close( transfer_function );
  texture_close( voxel_texture );
  texture_close( text_texture );
  shader_close( tri_prog );
  shader_close( ui_shaders );
  shader_close( raytrace_prog );
  shader_close( text_shaders );
  render_close();
  return 0;
}
#endif