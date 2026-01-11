#include "linalg.h"
#include "render_boundary.h"



entity primitive_box2d( arena *vbuffer, arena *ebuffer, fvec4 color )
{
  // Initialize output
  entity output = {};
  // Vertices
  vertex1 verts[4] = {};
  verts[0].pos = fvec3_init(-1.0f, -1.0f, 0.0f);
  verts[1].pos = fvec3_init( 1.0f, -1.0f, 0.0f);
  verts[2].pos = fvec3_init( 1.0f,  1.0f, 0.0f);
  verts[3].pos = fvec3_init(-1.0f,  1.0f, 0.0f);
  verts[0].col = fvec4_init( color.x, color.y, color.z, color.w);
  verts[1].col = fvec4_init( color.x, color.y, color.z, color.w);
  verts[2].col = fvec4_init( color.x, color.y, color.z, color.w);
  verts[3].col = fvec4_init( color.x, color.y, color.z, color.w);
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
  vertex1 *vtemp = arena_push_array( vbuffer, vert_count, vertex1 );
  u32     *etemp = arena_push_array( ebuffer, elem_count, u32 );
  memcpy( vtemp, verts, sizeof(verts) );
  memcpy( etemp, elems, sizeof(elems) );
  return output;
}


entity primitive_pyramid( arena *vbuffer, arena *ebuffer, fvec4 color )
{
  // Initialize output
  entity output = {};
  // Vertices: 4 base corners + 1 apex
  vertex1 verts[5] = {};
  // Base vertices (Y = -1)
  verts[0].pos = fvec3_init(-1.0f, -1.0f, -1.0f);  // back-left
  verts[1].pos = fvec3_init( 1.0f, -1.0f, -1.0f);  // back-right
  verts[2].pos = fvec3_init( 1.0f, -1.0f,  1.0f);  // front-right
  verts[3].pos = fvec3_init(-1.0f, -1.0f,  1.0f);  // front-left
  // Apex (Y = 1)
  verts[4].pos = fvec3_init( 0.0f,  1.0f,  0.0f);  // top

  // Colors
  verts[0].col = fvec4_init( 1.0f, 0.0f, 0.0f, 1.0f);  // red
  verts[1].col = fvec4_init( 0.0f, 1.0f, 0.0f, 1.0f);  // green
  verts[2].col = fvec4_init( 0.0f, 0.0f, 1.0f, 1.0f);  // blue
  verts[3].col = fvec4_init( 1.0f, 1.0f, 0.0f, 1.0f);  // yellow
  verts[4].col = fvec4_init( 1.0f, 1.0f, 1.0f, 1.0f);  // white

  // Elements: 4 triangular sides + 2 base triangles (CCW when viewed from outside)
  u32 elems[18] = {
    // Base (looking up from below, CCW)
    0, 2, 1,  // base tri 1
    0, 3, 2,  // base tri 2
    // Side faces (looking from outside, CCW)
    0, 1, 4,  // back face
    1, 2, 4,  // right face
    2, 3, 4,  // front face
    3, 0, 4,  // left face
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
  vertex1 *vtemp = arena_push_array( vbuffer, vert_count, vertex1 );
  u32     *etemp = arena_push_array( ebuffer, elem_count, u32 );
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
