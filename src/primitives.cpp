#include "app_data.h"


struct entity
{
  u64 vert_start;
  u64 elem_start;
  u64 count;
};

entity primitive_box2d( arena *vbuffer, arena *ebuffer, fvec4 color )
{
  // Initialize output
  entity output = {};
  // Vertices
  f32 verts[36] = {
    // position          // color (RGBA)          // Texture 
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,  // low left
     1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f, 0.0f,  // low right
     1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  // top right
    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f,  // top left
  };
  // Set the color
  {
    // Color vertex 1
    verts[3] = color.x;
    verts[4] = color.y;
    verts[5] = color.z;
    verts[6] = color.w;
    // Color vertex 2
    verts[12] = color.x;
    verts[13] = color.y;
    verts[14] = color.z;
    verts[15] = color.w;
    // Color vertex 3
    verts[21] = color.x;
    verts[22] = color.y;
    verts[23] = color.z;
    verts[24] = color.w;
    // Color vertex 4
    verts[30] = color.x;
    verts[31] = color.y;
    verts[32] = color.z;
    verts[33] = color.w;
  }
  u32 elems[6] = {
    0, 1, 2, // tri 1
    0, 2, 3, // tri 2
  };
  // Get and set vertex and element starting position in arena
  u32 vert_count = ARRAY_COUNT(verts);
  u32 elem_count = ARRAY_COUNT(elems);
  u32 verts_loaded_count = vbuffer->offset_new / sizeof(vertex1);
  u32 elems_loaded_count = ebuffer->offset_new / sizeof(u32);
  output.vert_start = verts_loaded_count;
  output.elem_start = elems_loaded_count;
  output.count = elem_count; 
  // Load data into arena
  f32 *vtemp = arena_push_array( vbuffer, vert_count, f32 );
  u32 *etemp = arena_push_array( ebuffer, elem_count, u32 );
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
