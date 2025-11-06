#pragma once

#include "core.h"
#include "platform.h"

struct shaders;
struct render_buffer;

typedef shaders* shaders_ptr;


void          render_init(arena *a);
void          render_resize(i32 width, i32 height);
// void          render_buffer_init();
shaders_ptr   render_triangle(arena *a);
void          render_draw(shaders_ptr s);
void          render_close();


void          frame_init();
void          frame_render();