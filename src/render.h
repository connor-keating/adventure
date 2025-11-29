#pragma once

#include "core.h"
#include "platform.h"


// You define these internally.
struct shaders;
struct render_buffer;
struct texture;

// Pointer types for the internal structs.
typedef shaders* shaders_ptr;
typedef render_buffer* rbuffer_ptr;
typedef texture* texture_ptr;


enum texture_dimension
{
  ONE,
  TWO,
  THREE
};

enum shader_type
{
  VERTEX,
  PIXEL,
  COMPUTE
};

enum buffer_type
{
  BUFF_VERTS,
  BUFF_ELEMS, 
  BUFF_CONST
};

struct vert_texture
{
  f32 x, y, z, u, v;
};

void          render_init(arena *a);
void          render_resize(i32 width, i32 height);
void          render_close();

rbuffer_ptr   render_buffer_init(arena *a, buffer_type t, void* data, u32 stride, u32 byte_count);
void          render_buffer_close( rbuffer_ptr b );
rbuffer_ptr   render_buffer_dynamic_init(arena *a, buffer_type t, void *data, u32 stride, u32 byte_count);
void          render_buffer_update(rbuffer_ptr buffer, void* data, u32 byte_count);
void          render_constant_set( rbuffer_ptr b, u32 slot );

void          render_text_init(arena *a);

void          render_draw(rbuffer_ptr vertex_buffer, shaders_ptr s, u32 count);
void          render_draw_elems(rbuffer_ptr vbuffer, rbuffer_ptr ebuffer, shaders_ptr s, u32 count, u32 elem_start, u32 vert_start);
void          render_draw_ui(rbuffer_ptr vertex_buffer, shaders_ptr s, u32 count);
void          render_draw_ui_elems(rbuffer_ptr vbuffer, rbuffer_ptr ebuffer, shaders_ptr s, u32 count, u32 elem_start, u32 vert_start);

texture_ptr   texture1d_init(arena *a, void* data, i32 width);
texture_ptr   texture2d_init(arena *a, void* pixels, i32 width, i32 height, i32 channels);
texture_ptr   texture3d_init(arena *a, void* data, i32 width, i32 height, i32 depth);
void          texture_bind(texture *tex, u32 slot);
void          texture_close(texture *tex);


shaders_ptr   shader_init(arena *a);
void          shader_load(shaders *s, shader_type t, const char *file, const char *entry, const char *target);
void          shader_close(shaders *s);

void          frame_init();
void          frame_render();