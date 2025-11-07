#pragma once

#include "core.h"
#include "platform.h"


enum shader_type
{
  VERTEX,
  PIXEL,
  COMPUTE
};

enum buffer_type
{
  VERTS,
  ELEMS
};


struct shaders;
struct render_buffer;

typedef shaders* shaders_ptr;
typedef render_buffer* rbuffer_ptr;


void          render_init(arena *a);
void          render_resize(i32 width, i32 height);
rbuffer_ptr   render_buffer_init(arena *a, buffer_type t, void* data, u32 stride, u32 byte_count);
void          render_draw(rbuffer_ptr vertex_buffer, shaders_ptr s, u32 count);
void          render_draw_elems(rbuffer_ptr vbuffer, rbuffer_ptr ebuffer, shaders_ptr s, u32 count, u32 elem_start, u32 vert_start);
void          render_close();

shaders_ptr   shader_init(arena *a);
void shader_load(shaders *s, shader_type t, const char *file, const char *entry, const char *target);

void          frame_init();
void          frame_render();