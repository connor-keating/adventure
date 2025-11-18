
void primitive_box2d( arena *vbuffer, arena *ebuffer )
{
  // Vertices
  f32 verts[36] = {
    // position          // color (RGBA)              // Texture
    -1.0f, -1.0f, 0.5f,  1.0f, 1.0f, 0.0f, 1.0f,  0.0f, 1.0f,  // low left
    -1.0f,  1.0f, 0.5f,  1.0f, 0.0f, 1.0f, 1.0f,  0.0f, 0.0f, // up  left
     1.0f, -1.0f, 0.5f,  0.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f, // low right
     1.0f,  1.0f, 0.5f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f, // up  right
  };
  // Elements
  u32 elems[6] = {
    // D3D11 is counter-clockwise winding order for front.
    0, 1, 2, // tri 1
    1, 3, 2, // tri 2
  };
  f32 *vtemp = arena_push_array( vbuffer, 36, f32 );
  u32 *etemp = arena_push_array( ebuffer,  6, u32 );
  memcpy( vtemp, verts, sizeof(verts) );
  memcpy( etemp, elems, sizeof(elems) );
}

void primitive_box3d( arena *vbuffer, arena *ebuffer )
{
  // Vertices 
  f32 verts[36] = {
    // position          // color (RGBA)              // Texture
    -1.0f, -1.0f, 0.5f,  1.0f, 1.0f, 0.0f, 1.0f,  0.0f, 1.0f,  // low left
    -1.0f,  1.0f, 0.5f,  1.0f, 0.0f, 1.0f, 1.0f,  0.0f, 0.0f, // up  left
     1.0f, -1.0f, 0.5f,  0.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f, // low right
     1.0f,  1.0f, 0.5f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f, // up  right
  };
  // Elements
  u32 elems[6] = {
    // D3D11 is counter-clockwise winding order for front.
    0, 1, 2, // tri 1
    1, 3, 2, // tri 2
  };
  f32 *vtemp = arena_push_array( vbuffer, 216, f32 );
  u32 *etemp = arena_push_array( ebuffer,  36, u32 );
  memcpy( vtemp, verts, sizeof(verts) );
  memcpy( etemp, elems, sizeof(elems) );
}
