#include "application.h"

#include "linalg.cpp"
#include "collision.cpp"
#include "input.h"
#include "platform.h"
#include "render.h"
#include "primitives.cpp"
#include "app_data.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAX_COUNT_SHADERS  100
#define MAX_COUNT_VERTEX   1000
#define MAX_COUNT_ENTITIES 100


struct camera
{
  glm::mat4 view;            // 64 bytes
  glm::mat4 proj;            // 64 bytes
  glm::vec3 pos;                 // 12 bytes
  f32 _padding;                  // 4 bytes → pad to 16-byte multiple
};


struct entities
{
  u64 total;
  u64 vert_start[MAX_COUNT_ENTITIES];
  u64 elem_start[MAX_COUNT_ENTITIES];
  u64 elem_count[MAX_COUNT_ENTITIES];
  glm::mat4 world_transforms[MAX_COUNT_ENTITIES];
};


struct appstate
{
  platform_window     window;
  arena               vbuffer_cpu; // Vertex buffer
  arena               ebuffer_cpu; // Element buffer
  rbuffer            *vbuffer_gpu;
  rbuffer            *ebuffer_gpu;
  rbuffer            *cam_ui_gpu;
  rbuffer            *cam_game_gpu;
  rbuffer            *world_gpu;
  input_state         inputs[KEY_COUNT];
  u64                 shader[MAX_COUNT_SHADERS];
  entities            entity;
};

global appstate *state;


internal void entity_load(entity e, glm::mat4 world)
{
  state->entity.vert_start[state->entity.total] = e.vert_start;
  state->entity.elem_start[state->entity.total] = e.elem_start;
  state->entity.elem_count[state->entity.total] = e.count;
  state->entity.world_transforms[state->entity.total] = world;
  state->entity.total++;
}


internal void input_reset( input_state *map )
{
  for (i32 i = 0; i < KEY_COUNT; ++i)
  {
    input_state current = map[i];
    map[i] = (current == INPUT_RELEASED) ? INPUT_UP : current;
  }
}


internal void texture_read(const char *filename, arena *a)
{
  // TODO: This is broken but I'm keeping it for when I need it.
  i32 test = platform_file_exists(filename);
  i32 x, y, n;
  i32 components_per_pixel = 4; // Force RGBA
  unsigned char *data = stbi_load(filename, &x, &y, &n, components_per_pixel);
  // texture_ptr tex = texture2d_init( a, data, x, y, components_per_pixel);
}


internal bool ui_button( fvec2 cursor, fvec3 pos, fvec3 scale, fvec4 color)
{
  bool is_clicked = false;
  entity button = primitive_box2d( &state->vbuffer_cpu, &state->ebuffer_cpu, color );
  fvec2 box_pos   = fvec2_init(pos.x, pos.y);
  fvec2 box_shape = fvec2_init(scale.x, scale.y);
  bool intersecting = point_in_rect( cursor, box_pos, box_shape );
  if ( (intersecting) && (state->inputs[KEY_SELECT] == INPUT_RELEASED) )
  {
    is_clicked = true;
  }
  glm::mat4 identity = glm::mat4(1.0f);
  glm::mat4 t = glm::translate( 
    identity, 
    glm::vec3( pos.x, pos.y, pos.z )
  );
  glm::mat4 s = glm::scale( 
    identity, 
    glm::vec3(scale.x, scale.y, scale.z)
  );
  entity_load( button, (t*s) );
  return is_clicked;
}


bool app_is_running()
{
  return platform_is_running();
}


arena app_init()
{
  // Allocate all program memory upfront.
  #if _DEBUG
    void *memory_base = (void*)Terabytes(2);
  #else
    void *memory_base = 0;
  #endif
  size_t memory_size = (size_t) Gigabytes(5);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena app_memory = arena_init(raw_memory, memory_size);
  arena *memory = &app_memory;
  // Create internal global state
  state = arena_push_struct(memory, appstate);
  // Start the platform layer
  platform_init(memory);
  // Create a window for the application
  state->window = platform_window_init();
  // Initialize renderer
  render_init(memory);
  render_data_init( memory, MAX_COUNT_SHADERS );
  // Begin render buffers
  state->vbuffer_cpu = subarena_init( memory, MAX_COUNT_VERTEX * sizeof(vertex1) );
  state->ebuffer_cpu = subarena_init( memory, MAX_COUNT_VERTEX * sizeof(u32) );
  // Vertex stride: float4 position (16 bytes) + float2 texcoord (8 bytes) = 24 bytes
  state->vbuffer_gpu = rbuffer_dynamic_init( memory, BUFF_VERTS, state->vbuffer_cpu.buffer, sizeof(vertex1), state->vbuffer_cpu.length);
  state->ebuffer_gpu = rbuffer_dynamic_init( memory, BUFF_ELEMS, state->ebuffer_cpu.buffer, sizeof(u32), state->ebuffer_cpu.length);
  // Shaders
  state->shader[0] = shader_init( memory );
  shader_load( state->shader[0], VERTEX, "shaders/ui.hlsl", "VSMain", "vs_5_0");
  shader_load( state->shader[0], PIXEL,  "shaders/ui.hlsl", "PSMain", "ps_5_0");
  // Cameras
  state->cam_ui_gpu   = rbuffer_dynamic_init( memory, BUFF_CONST, nullptr, 0, sizeof(camera) );
  state->cam_game_gpu = rbuffer_dynamic_init( memory, BUFF_CONST, nullptr, 0, sizeof(camera) );
  // Start the window
  platform_window_show();
  return app_memory;
}


void app_update(arena *a)
{
  // Reset input from last frame...
  input_reset(state->inputs);
  // Read all platform messages for this frame
  platform_message_process(&state->window, state->inputs);
  fvec2 cursor = {};
  platform_cursor_client_position( &cursor.x, &cursor.y, state->window.width, state->window.height);
  if (state->inputs[KEY_ESCAPE] == INPUT_DOWN)
  {
    platform_window_close();
  }
  // Reset buffers
  arena_free_all( &state->vbuffer_cpu );
  arena_free_all( &state->ebuffer_cpu );
  // Reset entity count
  state->entity.total = 0;
  // Game logic
  static fvec4 frame_background = fvec4_init(0.0f, 0.0f, 0.0f, 1.0f);
  entity uibox = primitive_pyramid( &state->vbuffer_cpu, &state->ebuffer_cpu, fvec4_init(1.0f, 0.0f, 0.0f, 1.0f) );
  // Set the UI camera
  f32 aspect = (f32)state->window.width / (f32)state->window.height;
  f32 half_height = 0.5f * state->window.height;
  f32 half_width = half_height * aspect;
  glm::mat4 identity = glm::mat4(1.0f);
  camera uicam = {};
  uicam.view = identity;
  uicam.proj = glm::ortho( -half_width, half_width, -half_height, half_height, 0.0f, 1.0f );
  uicam.pos  = glm::vec3(0.0f, 0.0f, 0.0f);
  camera game_cam = {};
  game_cam.pos = glm::vec3(0.0f, 0.0f, -3.0f);
  game_cam.view = glm::lookAt(game_cam.pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  game_cam.proj = glm::perspective( 45.0f, aspect, 0.1f, 100.0f);
  render_constant_set( state->cam_game_gpu, 0 );
  rbuffer_update( state->cam_game_gpu, &game_cam, sizeof(game_cam) );
  // Update vertex and element buffers
  rbuffer_update( state->vbuffer_gpu, state->vbuffer_cpu.buffer, state->vbuffer_cpu.offset_new );
  rbuffer_update( state->ebuffer_gpu, state->ebuffer_cpu.buffer, state->ebuffer_cpu.offset_new );
  // Begin frame rendering
  frame_init(frame_background.array);
  // Set vertex buffer
  rbuffer_vertex_set( 0, state->vbuffer_gpu );
  // Set index buffer
  rbuffer_index_set( state->ebuffer_gpu );
  // Set shader
  shader_set( state->shader[0] );
  // Draw
  render_draw_elems( 6, 0, 0 );
  frame_render();
}
