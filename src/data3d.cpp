
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


mesh model_load_obj(const char *file, arena *memory)
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
  model.vertices = arena_alloc_array(memory, model.vert_count, vertex);
  model.indices = arena_alloc_array(memory, model.index_count, u32);

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
