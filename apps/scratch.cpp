#include "application.h"

#include "linalg.cpp"
#include "input.h"
#include "platform.h"
#include "render.h"
#include "app_data.h"
#include "primitives.cpp"

#define MAX_COUNT_SHADERS 100
#define MAX_COUNT_VERTEX  1000


struct appstate
{
  bool             is_running;
  platform_window  window;
  arena           vbuffer_cpu; // Vertex buffer
  arena           ebuffer_cpu; // Element buffer
  rbuffer        *vbuffer_gpu;
  rbuffer        *ebuffer_gpu;
  input_state      inputs[KEY_COUNT];
  u64              shader[MAX_COUNT_SHADERS];
};

global appstate *state;

bool app_is_running()
{
  state->is_running = platform_is_running();
  return state->is_running;
}

void app_init(arena *memory)
{
  // Create internal global state
  state = arena_push_struct(memory, appstate);
  state->is_running = false;
  // Start the platform layer
  platform_init(memory);
  // Create a window for the application
  state->window = platform_window_init();
  // Initialize renderer
  render_init(memory);
  render_data_init( memory, MAX_COUNT_SHADERS );
  // Begin render buffers
  state->vbuffer_cpu = subarena_init( memory, MAX_COUNT_VERTEX * sizeof(fvec4) );
  state->ebuffer_cpu = subarena_init( memory, MAX_COUNT_VERTEX * sizeof(u32) );
  state->vbuffer_gpu = rbuffer_dynamic_init( memory, BUFF_VERTS, state->vbuffer_cpu.buffer, sizeof(fvec4), state->vbuffer_cpu.length);
  state->ebuffer_gpu = rbuffer_dynamic_init( memory, BUFF_ELEMS, state->ebuffer_cpu.buffer, sizeof(u32), state->ebuffer_cpu.length);
  // Load UI thing
  fvec4 colors[2] = {
    fvec4_init(1.0f, 0.0f, 0.0f, 1.0f), // red
    fvec4_init(0.0f, 1.0f, 0.0f, 1.0f), // green
  };
  rbuffer* color_buffer = rbuffer_init(memory, BUFF_VERTS, colors, sizeof(fvec4), sizeof(colors) );
  rbuffer_vertex_set( 1, color_buffer );
  glm::mat4 identity = glm::mat4(1.0f);
  glm::mat4 shrink = glm::scale(identity, glm::vec3(0.2f, 0.2f, 0.2f) );
  glm::mat4 t = glm::translate( identity, glm::vec3( 0.0f, 0.5f, 0.0f) );
  glm::mat4 model1 = t * shrink;
  glm::mat4 model2 = glm::scale(
    glm::translate( identity, glm::vec3( 0.0f,-0.5f, 0.0f) ), 
    glm::vec3(0.2f)
  );
  glm::mat4 worlds[2] ={
    model1,
    model2
  };
  rbuffer* world_buffer = rbuffer_init(memory, BUFF_VERTS, worlds, sizeof(glm::mat4), sizeof(worlds) );
  rbuffer_vertex_set( 2, world_buffer );
  // Shaders
  state->shader[0] = shader_init( memory );
  shader_load( state->shader[0], VERTEX, "shaders/simple.hlsl", "VSMain", "vs_5_0");
  shader_load( state->shader[0], PIXEL,  "shaders/simple.hlsl", "PSMain", "ps_5_0");
  // Start the window
  platform_window_show();
}

void app_update(arena *a)
{
  platform_message_process(&state->window, state->inputs);
  if (state->inputs[KEY_ESCAPE] == INPUT_DOWN)
  {
    platform_window_close();
  }
  // Reset buffers
  arena_free_all( &state->vbuffer_cpu );
  arena_free_all( &state->ebuffer_cpu );
  // Begin frame
  fvec4 frame_background = fvec4_init(0.0f, 0.0f, 0.0f, 1.0f);
  frame_init(frame_background.array);

  model3d uibox = primitive_box2d( &state->vbuffer_cpu, &state->ebuffer_cpu, fvec4_uniform(0.0f) );
  rbuffer_update( state->vbuffer_gpu, state->vbuffer_cpu.buffer, state->vbuffer_cpu.length );
  rbuffer_update( state->ebuffer_gpu, state->ebuffer_cpu.buffer, state->ebuffer_cpu.length );
  rbuffer_vertex_set( 0, state->vbuffer_gpu );
  rbuffer_index_set( state->ebuffer_gpu );
  shader_set( state->shader[0] );
  // render_draw_instances( 3, 2 );

  render_draw_instances_elems( 
    uibox.count, 
    2
  );

  // End frame
  frame_render();
}