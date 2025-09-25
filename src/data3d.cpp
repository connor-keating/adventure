
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


internal mesh bbox_create(fvec3 min, fvec3 max, arena *vert_buffer, arena *elem_buffer)
{
  mesh bbox = {};
  bbox.vert_count = 8;
  bbox.index_count = 24;
  bbox.vertices = arena_push_array(vert_buffer, bbox.vert_count, vertex);
  bbox.indices = arena_push_array(elem_buffer, bbox.index_count, u32);
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


mesh primitive_cube(arena *a)
{
  mesh output = {};
  f32 verts[] = {
    // positions      
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
  };
  u32 indices[] = {
    0,1, 1,2, 2,3, 3,0,        // bottom
    4,5, 5,6, 6,7, 7,4,        // top
    0,4, 1,5, 2,6, 3,7         // verticals
  };
  output.vert_count = ARRAY_COUNT(verts);
  output.index_count = ARRAY_COUNT(indices);
  output.vertices = arena_push_array(a, output.vert_count, vertex);
  output.indices = arena_push_array(a, output.index_count, u32);
  std::memcpy(output.vertices, verts, sizeof(verts)); 
  std::memcpy(output.indices, indices, sizeof(indices)); 
  return output;
}


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
  fvec3 min = model_min(model);
  fvec3 max = model_max(model);
  mesh bbox = bbox_create(min, max, vert_buffer, elem_buffer);
  return bbox;
}


i64 model_starting_offset(arena *a, mesh model)
{
  // Note: this is the CPU memory, not the GPU memory. The two buffers could be in synch, but that's not guarenteed.
  i64 offset = (address)model.indices - (address) a->buffer;
  return offset;
}


void voxel_grid_init(arena *a, fvec3 counts)
{
  u32 cube_count = (u32) (counts.x * counts.y * counts.z);
  // The initial cube is [-1, 1]
  f32 x_scale = 1 / counts.x;
  f32 y_scale = 1 / counts.y;
  f32 z_scale = 1 / counts.z;
  u32 row_elem_count = counts.x;
  u32 slice_elem_count = counts.x * counts.y;
  f32 cube_min = -1.0f;
  glm::mat4 *modelmats = arena_push_array(a, cube_count, glm::mat4); 
  // Create instance transforms
  for (int k = 0; k < counts.z; k++)
  {
    for (int j = 0; j < counts.y; j++)
    {
      for (int i = 0; i < counts.x; i++)
      {
        // Transformations are applied in reverse multiplication order.
        glm::mat4 transform = glm::mat4(1.0f);
        f32 pos_x = cube_min + (2*i + 1) * x_scale;  // voxel center x
        f32 pos_y = cube_min + (2*j + 1) * y_scale;  // voxel center y
        f32 pos_z = cube_min + (2*k + 1) * z_scale;  // voxel center z
        transform = glm::translate(transform, glm::vec3(pos_x, pos_y, pos_z));
        transform = glm::scale(transform, glm::vec3(x_scale, y_scale, z_scale));
        u32 element = i + (j * row_elem_count) + (k * slice_elem_count); 
        modelmats[element] = transform;
      }
    }
  }
  // Keep transforms in storage buffer
  size_t transform_buffer_size = cube_count * sizeof(glm::mat4);
  shader_storage_init(0, (void*)&modelmats[0], transform_buffer_size);
}


mesh model_voxelize(mesh model, u32 resolution, arena *vert_buffer, arena *elem_buffer, arena *scratch)
{
  // First create a bbox that is a cube of the maximum distance of the raw bbox.
  fvec3 min = model_min(model);
  fvec3 max = model_max(model);
  // Output initialization
  fvec3 bbox_min = min;
  fvec3 bbox_max = max;
  fvec3 lengths = fvec3_sub(max, min);
  f32 max_length = fvec3_max_elem(lengths);
  if (max_length != lengths.x)
  {
    f32 delta = max_length - lengths.x; // compute differences between largest length and current length.
    f32 padding = delta / 2.0f; // Half of the total padding.
    bbox_min.x = min.x - padding; // Apply padding before model min.
    bbox_max.x = max.x + padding; // Apply padding after model max.
  }
  if (max_length != lengths.y)
  {
    f32 delta = max_length - lengths.y; // compute differences between largest length and current length.
    f32 padding = delta / 2.0f; // Half of the total padding.
    bbox_min.y = min.y - padding; // Apply padding before model min.
    bbox_max.y = max.y + padding; // Apply padding after model max.
  }
  if (max_length != lengths.z)
  {
    f32 delta = max_length - lengths.z; // compute differences between largest length and current length.
    f32 padding = delta / 2.0f; // Half of the total padding.
    bbox_min.z = min.z - padding; // Apply padding before model min.
    bbox_max.z = max.z + padding; // Apply padding after model max.
  }

  // TODO: Is this the best fix?
  // In case a triangle is axis-aligned and lies on a voxel edge, it may or may not be counted.
  f32 offset = (1 / 10001.0f);
  fvec3 epsilon = fvec3_scale(fvec3_sub(bbox_max, bbox_min), offset);
  bbox_min = fvec3_sub(bbox_min, epsilon);
  bbox_max = fvec3_add(bbox_max, epsilon);

  // Create mesh
  mesh bbox = bbox_create(bbox_min, bbox_max, vert_buffer, elem_buffer);
  
  // Calculate voxel units
  fvec3 units = fvec3_scale(fvec3_sub(bbox_max, bbox_min), (1/resolution));

  // Create an array that contains the enabled voxels
  u32 count = resolution * resolution * resolution;
  u8 *voxels = arena_push_array(scratch, count, u8);

  // Set all verts of the model to its cube bbox min
  vertex *verts_new = arena_push_array(scratch, model.vert_count, vertex);
  // the cubed bbox_min
  // TODO: Should you update the model or make a copy?
  for (int i = 0; i < model.vert_count; ++i)
  {
    verts_new[i].pos = fvec3_sub(model.vertices[i].pos, bbox_min);
  }
  return bbox;
}
