
void primitive_box2d( arena *verts, arena *elems )
{
  f32 tri_verts[36] = {
    // position          // color (RGBA)              // Texture
    -1.0f, -1.0f, 0.5f,  1.0f, 1.0f, 0.0f, 1.0f,  0.0f, 1.0f,  // low left
    -1.0f,  1.0f, 0.5f,  1.0f, 0.0f, 1.0f, 1.0f,  0.0f, 0.0f, // up  left
     1.0f, -1.0f, 0.5f,  0.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f, // low right
     1.0f,  1.0f, 0.5f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f, // up  right
  };
  u32 tri_elems[6] = {
    // D3D11 is counter-clockwise winding order for front.
    0, 1, 2, // tri 1
    1, 3, 2, // tri 2
  };
  f32 *vtemp = arena_push_array( verts, 36, f32 );
  u32 *etemp = arena_push_array( elems,  6, u32 );
  memcpy( vtemp, tri_verts, sizeof(tri_verts) );
  memcpy( etemp, tri_elems, sizeof(tri_elems) );
}
