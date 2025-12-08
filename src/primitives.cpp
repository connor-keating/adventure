#include "render.h"
#include "app_data.h"


model3d primitive_box2d( arena *vbuffer, arena *ebuffer )
{
  // Initialize output
  model3d output = {};
  // Vertices
  f32 verts[36] = {
    // position          // color (RGBA)              // Texture
    -1.0f, -1.0f, 0.0f,  1.0f, 1.0f, 0.0f, 1.0f,  0.0f, 0.0f, // low left
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 0.0f, // low right
     1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f, // top right
    -1.0f,  1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f, // top left
  };
  // Elements
  u32 elems[6] = {
    0, 1, 2, // tri 1
    0, 2, 3, // tri 2
  };
  // Get and set vertex and element starting position in arena
  u32 elem_count = ARRAY_COUNT(elems);
  u32 verts_loaded_count = vbuffer->offset_new / sizeof(vertex1);
  u32 elems_loaded_count = ebuffer->offset_new / sizeof(u32);
  output.vert_start = verts_loaded_count;
  output.elem_start = elems_loaded_count;
  output.count = elem_count; 
  // Load data into arena
  f32 *vtemp = arena_push_array( vbuffer, 36, f32 );
  u32 *etemp = arena_push_array( ebuffer,  6, u32 );
  memcpy( vtemp, verts, sizeof(verts) );
  memcpy( etemp, elems, sizeof(elems) );
  return output;
}

model3d primitive_box2d_extra( arena *vbuffer, arena *ebuffer )
{
  // Initialize output
  model3d output = {};
  // Vertices
  f32 verts[36] = {
    // position          // color (RGBA)              // Texture
    -1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // low left
     1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f,  1.0f, 0.0f, // low right
     1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f, // top right
    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f, // top left
  };
  // Elements
  u32 elems[6] = {
    0, 1, 2, // tri 1
    0, 2, 3, // tri 2
  };
  // Get and set vertex and element starting position in arena
  u32 elem_count = ARRAY_COUNT(elems);
  u32 verts_loaded_count = vbuffer->offset_new / sizeof(vertex1);
  u32 elems_loaded_count = ebuffer->offset_new / sizeof(u32);
  output.vert_start = verts_loaded_count;
  output.elem_start = elems_loaded_count;
  output.count = elem_count; 
  // Load data into arena
  f32 *vtemp = arena_push_array( vbuffer, 36, f32 );
  u32 *etemp = arena_push_array( ebuffer,  6, u32 );
  memcpy( vtemp, verts, sizeof(verts) );
  memcpy( etemp, elems, sizeof(elems) );
  return output;
}


void primitive_box3d( arena *vbuffer, arena *ebuffer )
{
  // Vertices
  f32 verts[] = {
      // positions      // color fvec4           // tex fvec2
    -1.0f,-1.0f,-1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
     1.0f,-1.0f,-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
     1.0f, 1.0f,-1.0f,  0.0f, 1.0f, 0.0f, 1.0f,  0.0f, 0.0f,
    -1.0f, 1.0f,-1.0f,  0.0f, 0.0f, 1.0f, 1.0f,  0.0f, 0.0f,
    -1.0f,-1.0f, 1.0f,  1.0f, 1.0f, 0.0f, 1.0f,  0.0f, 0.0f,
     1.0f,-1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,  0.0f, 0.0f,
     1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
    -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f,
  };
  // Elements
  u32 elems[] = {
    0,1, 1,2, 2,3, 3,0,        // bottom
    4,5, 5,6, 6,7, 7,4,        // top
    0,4, 1,5, 2,6, 3,7         // verticals
  };
  u32 vert_count = sizeof(verts) / ( sizeof(vertex1) );
  u32 elem_count = ARRAY_COUNT(elems);
  vertex1 *vertices = arena_push_array(vbuffer, vert_count, vertex1);
  u32     *elements = arena_push_array(ebuffer, elem_count, u32);
  memcpy(vertices, verts, sizeof(verts));
  memcpy(elements, elems, sizeof(elems));
  /*
  f32 *vtemp = arena_push_array( vbuffer, 216, f32 );
  u32 *etemp = arena_push_array( ebuffer,  36, u32 );
  memcpy( vtemp, verts, sizeof(verts) );
  memcpy( etemp, elems, sizeof(elems) );
  */
}

void primitive_quad_fullscreen( arena *vbuffer, arena *ebuffer )
{
  // Fullscreen quad in NDC coordinates (-1 to 1)
  f32 verts[] = {
      // positions      // color fvec4           // tex fvec2
    -1.0f, -1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  // bottom-left
     1.0f, -1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  // bottom-right
     1.0f,  1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  // top-right
    -1.0f,  1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  // top-left
  };
  // Elements - two triangles
  u32 elems[] = {
    0, 1, 2,  // first triangle
    0, 2, 3   // second triangle
  };

  u32 vert_count = 4;
  u32 elem_count = 6;
  vertex1 *vertices = arena_push_array(vbuffer, vert_count, vertex1);
  u32     *elements = arena_push_array(ebuffer, elem_count, u32);
  memcpy(vertices, verts, sizeof(verts));
  memcpy(elements, elems, sizeof(elems));
}
