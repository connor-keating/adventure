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
  frame_init();
  // Reset render data buffers
  arena_free_all( &state->vbuffer_cpu );
  arena_free_all( &state->ebuffer_cpu );
  // Add raymarching quad to buffers
  model3d raybox2d = primitive_box2d( &state->vbuffer_cpu, &state->ebuffer_cpu );
  model3d button_test = primitive_box2d( &state->vbuffer_cpu, &state->ebuffer_cpu );
  // TODO: Update whole buffer or just what you need with offset_new?
  rbuffer_update( 
    state->vbuffer_gpu, 
    state->vbuffer_cpu.buffer, 
    state->vbuffer_cpu.length
  );
  rbuffer_update( 
    state->ebuffer_gpu, 
    state->ebuffer_cpu.buffer, 
    state->ebuffer_cpu.length 
  );
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
    raybox2d.vert_start, 
    raybox2d.elem_start
  );
  // Create UI elements
  glm::mat4 ui_world = glm::mat4(1.0f);
  ui_world = glm::scale(
    ui_world,
    glm::vec3(0.2f, 0.2f, 0.0f)
  );
  rbuffer_update( state->ui_matrix, &ui_world, sizeof(ui_world) );
  render_constant_set( state->ui_matrix, 1 );
  render_draw_ui_elems( 
    state->vbuffer_gpu, 
    state->ebuffer_gpu, 
    state->shader[0], 
    button_test.count, 
    0, // button_test.vert_start, 
    0 // button_test.elem_start
  );
  frame_render();
}
