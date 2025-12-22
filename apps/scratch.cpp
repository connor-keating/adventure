#include "application.h"

#include "linalg.cpp"
#include "input.h"
#include "platform.h"
#include "render.h"
#include "app_data.h"

#define MAX_COUNT_SHADERS 100
#define MAX_COUNT_VERTEX  1000


struct appstate
{
  bool             is_running;
  platform_window  window;
  rbuffer*         tribuffer;
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
  fvec4 tri[3] = {
    fvec4_init( 0.0f, -0.5f, 0.5f, 1.0f), // Vert1
    fvec4_init(-0.5f,  0.5f, 0.5f, 1.0f), // Vert2
    fvec4_init(-1.0f, -0.5f, 0.5f, 1.0f)  // Vert3
  };
  state->tribuffer = rbuffer_init(memory, BUFF_VERTS, tri, sizeof(fvec4), sizeof(tri) );
  fvec4 colors[2] = {
    fvec4_init(1.0f, 0.0f, 0.0f, 1.0f), // red
    fvec4_init(0.0f, 1.0f, 0.0f, 1.0f), // green
  };
  rbuffer* color_buffer = rbuffer_init(memory, BUFF_VERTS, colors, sizeof(fvec4), sizeof(colors) );
  rbuffer_vertex_set( 1, color_buffer );
  glm::mat4 worlds[2] ={
    glm::translate(glm::mat4(1.0f), glm::vec3( 0.0f, 0.5f, 0.0f) ),
    glm::translate(glm::mat4(1.0f), glm::vec3( 0.0f,-0.5f, 0.0f) )
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
  fvec4 frame_background = fvec4_init(0.0f, 0.0f, 0.0f, 1.0f);
  frame_init(frame_background.array);

  rbuffer_vertex_set( 0, state->tribuffer );
  shader_set( state->shader[0] );
  render_draw_instances( 3, 2 );

  frame_render();
}