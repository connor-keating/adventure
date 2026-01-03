#include "application.h"

#include "linalg.cpp"
#include "collision.cpp"
#include "input.h"
#include "platform.h"
#include "render.h"
#include "primitives.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAX_COUNT_SHADERS 100
#define MAX_COUNT_VERTEX  1000


struct camera
{
  glm::mat4 view;            // 64 bytes
  glm::mat4 proj;            // 64 bytes
  glm::vec3 pos;                 // 12 bytes
  f32 _padding;                  // 4 bytes → pad to 16-byte multiple
};


struct ui_data
{
  glm::mat4 world;
  f32 hot;
};


struct appstate
{
  platform_window  window;
  camera           cam_ui_cpu;
  arena           vbuffer_cpu; // Vertex buffer
  arena           ebuffer_cpu; // Element buffer
  rbuffer        *vbuffer_gpu;
  rbuffer        *ebuffer_gpu;
  rbuffer        *cam_ui_gpu;
  rbuffer*       world_buffer;
  input_state      inputs[KEY_COUNT];
  u64              shader[MAX_COUNT_SHADERS];
};

global appstate *state;


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


internal bool ui_button(ui_data *buttons, u32 *count, fvec2 cursor, fvec3 pos, fvec3 scale)
{
  bool is_clicked = false;
  scale = fvec3_div(scale, 2.0f);
  fvec2 point = fvec2_init(cursor.x, cursor.y);
  fvec2 box_pos = fvec2_init(pos.x, pos.y);
  fvec2 box_shape = fvec2_init(scale.x, scale.y);
  bool intersecting = point_in_rect( point, box_pos, box_shape );
  buttons[*count].hot = (f32) intersecting;
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
  buttons[*count].world = t*s;
  (*count)++;
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
  // Renderer camera
  glm::mat4 identity = glm::mat4(1.0f);
  state->cam_ui_cpu.view = identity;
  f32 aspect = (f32)state->window.width / (f32)state->window.height;
  f32 half_height = 0.5f * state->window.height;
  f32 half_width = half_height * aspect;
  state->cam_ui_cpu.proj = glm::ortho( -half_width, half_width, -half_height, half_height, 0.0f, 1.0f );
  // state->cam_ui_cpu.proj = glm::ortho( -5.0f, 5.0f, -5.0f, 5.0f, 0.0f, 1.0f );
  state->cam_ui_gpu = rbuffer_dynamic_init( memory, BUFF_CONST, &state->cam_ui_cpu, 0, sizeof(camera) );
  rbuffer_update( state->cam_ui_gpu, &state->cam_ui_cpu, sizeof(camera) );
  // Bind camera constant buffer to pixel shader
  render_constant_set( state->cam_ui_gpu, 0 );
  // Begin render buffers
  state->vbuffer_cpu = subarena_init( memory, MAX_COUNT_VERTEX * sizeof(fvec4) );
  state->ebuffer_cpu = subarena_init( memory, MAX_COUNT_VERTEX * sizeof(u32) );
  // Vertex stride: float4 position (16 bytes) + float2 texcoord (8 bytes) = 24 bytes
  state->vbuffer_gpu = rbuffer_dynamic_init( memory, BUFF_VERTS, state->vbuffer_cpu.buffer, 24, state->vbuffer_cpu.length);
  state->ebuffer_gpu = rbuffer_dynamic_init( memory, BUFF_ELEMS, state->ebuffer_cpu.buffer, sizeof(u32), state->ebuffer_cpu.length);
  // Load UI thing
  fvec4 colors[5] = {
    fvec4_init(1.0f, 1.0f, 1.0f, 1.0f), // white
    fvec4_init(1.0f, 0.0f, 0.0f, 1.0f), // red
    fvec4_init(0.0f, 1.0f, 0.0f, 1.0f), // green
    fvec4_init(0.0f, 0.0f, 1.0f, 1.0f), // blue
    fvec4_init(1.0f, 0.0f, 1.0f, 1.0f), // color
  };
  rbuffer* color_buffer = rbuffer_init(memory, BUFF_VERTS, colors, sizeof(fvec4), sizeof(colors) );
  rbuffer_vertex_set( 1, color_buffer );
  state->world_buffer = rbuffer_dynamic_init(memory, BUFF_VERTS, nullptr, sizeof(ui_data), 5*sizeof(ui_data) );
  rbuffer_vertex_set( 2, state->world_buffer );
  // Shaders
  state->shader[0] = shader_init( memory );
  shader_load( state->shader[0], VERTEX, "shaders/simple.hlsl", "VSMain", "vs_5_0");
  shader_load( state->shader[0], PIXEL,  "shaders/simple.hlsl", "PSMain", "ps_5_0");
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
  // Game logic
  static fvec4 frame_background = fvec4_init(0.0f, 0.0f, 0.0f, 1.0f);
  static i32 red = 0;
  static i32 grn = 0;
  static i32 blu = 0;
  u32 ui_count = 0;
  ui_data buttons[5] = {};
  model3d uibox = primitive_box2d( &state->vbuffer_cpu, &state->ebuffer_cpu, fvec4_uniform(0.0f) );
  // Locations
  f32 aspect = (f32)state->window.width / (f32)state->window.height;
  f32 half_height = 0.5 * state->window.height;
  f32 half_width = half_height * aspect;
  ui_button( buttons, &ui_count, cursor, fvec3_init(   cursor.x,    cursor.y, 0.0f), fvec3_uniform( 10.f) );
  bool red_clicked = ui_button( buttons, &ui_count, cursor, fvec3_init(-150.0f,   0.0f, 0.0f), fvec3_uniform( 100.f) );
  if (red_clicked)
  {
    red ^= 1;
    frame_background.x = red * 0.6f;
  }
  bool grn_clicked = ui_button( buttons, &ui_count, cursor, fvec3_init( 0.0f,     0.0f, 0.0f), fvec3_uniform( 100.f) );
  if (grn_clicked)
  {
    grn ^= 1;
    frame_background.y = grn * 0.6f;
  }
  bool blu_clicked = ui_button( buttons, &ui_count, cursor, fvec3_init( 150.0f,   0.0f, 0.0f), fvec3_uniform( 100.f) );
  if (blu_clicked)
  {
    blu ^= 1;
    frame_background.z = blu * 0.6f;
  } 
  rbuffer_update( state->vbuffer_gpu, state->vbuffer_cpu.buffer, state->vbuffer_cpu.length );
  rbuffer_update( state->ebuffer_gpu, state->ebuffer_cpu.buffer, state->ebuffer_cpu.length );
  rbuffer_update( state->world_buffer, buttons, sizeof(buttons) );
  rbuffer_vertex_set( 0, state->vbuffer_gpu );
  rbuffer_index_set( state->ebuffer_gpu );
  shader_set( state->shader[0] );
  // render_draw_instances( 3, 2 );
  frame_init(frame_background.array);
  render_draw_instances_elems( uibox.count, ui_count );
  frame_render();
}
