#pragma once

#include "core.h"
#include "platform.h"
#include "render_boundary.h"


// You define these internally.

struct shaders;
// Render buffer data.
struct rbuffer;
struct texture;

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


void       render_init(arena *a);
void       render_data_init( arena *a, u64 shader_count );
void       render_resize(i32 width, i32 height);
void       render_close();

rbuffer*   rbuffer_init(arena *a, buffer_type t, void* data, u32 stride, u32 byte_count);
void       rbuffer_close( rbuffer* b );
rbuffer*   rbuffer_dynamic_init(arena *a, buffer_type t, void *data, u32 stride, u32 byte_count);
void       rbuffer_update(rbuffer* buffer, void* data, u32 byte_count);
void       rbuffer_vertex_set( u32 slot_start, rbuffer *buffer );
void       rbuffer_vertex_describe( u64 shader_index, vertex_type vtype );
void       rbuffer_index_set( rbuffer *b );
void       render_constant_set( rbuffer* b, u32 slot );

void       render_text_init(arena *a);

void       render_draw( u32 count );
void       render_draw_elems( u32 count, u32 elem_start, u32 vert_start);
void       render_draw_instances( u32 vertex_count, u32 instance_count);
void       render_draw_instances_elems( u32 elem_count, u32 instance_count );
void       render_draw_ui( u32 count );
void       render_draw_ui_elems(rbuffer* vbuffer, rbuffer* ebuffer, u64 shader_index, u32 count, u32 elem_start, u32 vert_start);

texture*   texture1d_init(arena *a, void* data, i32 width);
texture*   texture2d_init(arena *a, void* pixels, i32 width, i32 height, i32 channels);
texture*   texture3d_init(arena *a, void* data, i32 width, i32 height, i32 depth);
void       texture_bind(texture *tex, u32 slot);
void       texture_close(texture *tex);


u64        shader_init(arena *a);
void       shader_load( u64 shader_index, shader_type t, const char *file, const char *entry, const char *target);
void       shader_set( u64 shader_index );
void       shader_close(shaders *s);

void       frame_init( f32 *background_color);
void       frame_render();