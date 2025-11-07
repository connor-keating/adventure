#pragma once

#include "core.h"
#include "platform.h"

struct shaders;
struct render_buffer;

typedef shaders* shaders_ptr;
typedef render_buffer* rbuffer_ptr;


void          render_init(arena *a);
void          render_resize(i32 width, i32 height);
rbuffer_ptr   render_buffer_init(arena *a);
shaders_ptr   render_triangle(arena *a);
void          render_draw(rbuffer_ptr vertex_buffer, shaders_ptr s);
void          render_close();


void          frame_init();
void          frame_render();