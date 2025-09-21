
// Application data types
struct vertex
{
  fvec3 pos;
};


struct mesh
{
  vertex *vertices;
  u32 *indices;
  u32 vert_count;
  u32 index_count;
};


mesh model_load_obj(const char *file, arena *vert_buffer, arena *elem_buffer)
{
  // Initialize output
  mesh model = {};
  // Parse the file and read that data from it.
  tinyobj_attrib_t attrib;
  tinyobj_shape_t* shapes = NULL;
  size_t shape_count;
  tinyobj_material_t* materials = NULL;
  size_t num_materials;
  u32 flags = TINYOBJ_FLAG_TRIANGULATE;
  i32 ret = tinyobj_parse_obj(
    &attrib,
    &shapes,
    &shape_count,
    &materials,
    &num_materials,
    file,
    get_file_data, // Required callback function read file as is.
    NULL,
    flags
  );
  ASSERT(ret == TINYOBJ_SUCCESS, "ERROR: Failed to load obj.");
  // We only have one shape
  u8 shape_idx = 0;
  tinyobj_shape_t *shape = &shapes[shape_idx];

  // Loop over all verts and determine how big our arrays should be

  // Allocate output memory
  // u64 unique_index = 0;
  u32 element = 0;
  model.vert_count = attrib.num_faces;
  model.index_count = attrib.num_faces;
  model.vertices = arena_push_array(vert_buffer, model.vert_count, vertex);
  model.indices = arena_push_array(elem_buffer, model.index_count, u32);

  // Metadata for loop
  u32 face_count = attrib.num_face_num_verts;
  i32 face_vert_count;// will use this when we're just iterating through verts
  i32 offset = 0;
  for (size_t face_idx = 0; face_idx < face_count; face_idx++)
  {
    // Get number of verts per face
    face_vert_count = attrib.face_num_verts[face_idx];
    // Loop over each vert
    for (size_t vert_idx = 0; vert_idx < face_vert_count; vert_idx++)
    {
      element = offset + vert_idx;
      tinyobj_vertex_index_t vert = attrib.faces[element];
      u32 vert_offset = 3 * vert.v_idx;
      u32 text_offset = 2 * vert.vt_idx;
      vertex data = {};
      data.pos.x = attrib.vertices[vert_offset+0];
      data.pos.y = attrib.vertices[vert_offset+1];
      data.pos.z = attrib.vertices[vert_offset+2];
      // data.texture_coords.x = attrib.texcoords[text_offset+0];
      // data.texture_coords.y = 1.0f - attrib.texcoords[text_offset+1];
      model.vertices[element] = data;
      model.indices[element] = element;
      // unique_index++;
    }
    offset += face_vert_count;
  }
  return model;
}


fvec3 model_max(mesh model)
{
  fvec3 max = {};
  u32 count = model.vert_count;
  for (i32 i = 0; i < count; i++)
  {
    max = fvec3_max( max, model.vertices[i].pos );
  }
  return max;
}


fvec3 model_min(mesh model)
{
  fvec3 min = {};
  u32 count = model.vert_count;
  for (i32 i = 0; i < count; i++)
  {
    min = fvec3_min( min, model.vertices[i].pos );
  }
  return min;
}


fvec3 model_centroid(mesh model)
{
  // Calculate AABB centroid.
  fvec3 center = {};
  fvec3 max = {};
  fvec3 min = {};
  u32 count = model.vert_count;
  for (i32 i = 0; i < count; i++)
  {
    max = fvec3_max( max, model.vertices[i].pos );
    min = fvec3_min( min, model.vertices[i].pos );
  }
  center = fvec3_scale(fvec3_add(max, min), 0.5);
  return center;
}


mesh model_bbox_add(arena *vert_buffer, arena *elem_buffer, mesh model)
{
  mesh bbox = {};
  bbox.vert_count = 8;
  bbox.index_count = 24;
  bbox.vertices = arena_push_array(vert_buffer, bbox.vert_count, vertex);
  bbox.indices = arena_push_array(elem_buffer, bbox.index_count, u32);
  fvec3 min = model_min(model);
  fvec3 max = model_max(model);
  // Bottom
  bbox.vertices[0].pos = min;
  bbox.vertices[1].pos = fvec3{ {max.x, min.y, min.z} };
  bbox.vertices[2].pos = fvec3{ {max.x, min.y, max.z} };
  bbox.vertices[3].pos = fvec3{ {min.x, min.y, max.z} };
  // Top
  bbox.vertices[4].pos = fvec3{ {min.x, max.y, min.z} };
  bbox.vertices[5].pos = fvec3{ {max.x, max.y, min.z} };
  bbox.vertices[6].pos = fvec3{ {max.x, max.y, max.z} };
  bbox.vertices[7].pos = fvec3{ {min.x, max.y, max.z} };
  // Indices
  const u32 indices[24] = {
    0,1, 1,2, 2,3, 3,0,
    4,5, 5,6, 6,7, 7,4,
    0,4, 1,5, 2,6, 3,7
  };
  std::memcpy(bbox.indices, indices, sizeof(indices)); 
  return bbox;
}


i64 model_starting_offset(arena *a, mesh model)
{
  // Note: this is the CPU memory, not the GPU memory. The two buffers could be in synch, but that's not guarenteed.
  i64 offset = (address)model.indices - (address) a->buffer;
  return offset;
}
