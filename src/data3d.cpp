
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


struct voxel_grid
{
  u8 *contents;
  fvec3 min;
  fvec3 max;
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


// Is the triangle winding order counter-clockwise order?
internal bool triangle_is_ccw(fvec2 v0, fvec2 v1, fvec2 v2)
{
  fvec2 e0 = fvec2_sub(v1, v0);
  fvec2 e1 = fvec2_sub(v2, v0);
  f32 result = cross2(e0, e1);
  // If result > 0 it is counter-clockwise
  return (result > 0);
}


internal f32 voxel_x_get(fvec3 n, fvec3 v0, fvec2 point)
{
  // return ( -(tri_normal.y * (point.x - v0.y) + tri_normal.z * (point.y - v0.z) ) / tri_normal.x + v0.x);
  return (-(n.y * (point.x - v0.y) + n.z * (point.y - v0.z)) / n.x + v0.x);
}


internal bool top_left_edge(fvec2 v0, fvec2 v1)
{
  // return ( (v1.y < v0.y) || (v1.y == v0.y && v0.x > v1.x) );
  return ((v1.y < v0.y) || (v1.y == v0.y && v0.x > v1.x));
}

int check_point_triangle(fvec2 v0, fvec2 v1, fvec2 v2, fvec2 point) {
  f32 float_error = 0.000001;
  fvec2 PA = fvec2_sub(point, v0);
  fvec2 PB = fvec2_sub(point, v1);
  fvec2 PC = fvec2_sub(point, v2);

  float t1 = PA.x * PB.y - PA.y * PB.x;
  if (std::fabs(t1) < float_error && PA.x * PB.x <= 0 && PA.y * PB.y <= 0)
    return 1;

  float t2 = PB.x * PC.y - PB.y * PC.x;
  if (std::fabs(t2) < float_error && PB.x * PC.x <= 0 && PB.y * PC.y <= 0)
    return 2;

  float t3 = PC.x * PA.y - PC.y * PA.x;
  if (std::fabs(t3) < float_error && PC.x * PA.x <= 0 && PC.y * PA.y <= 0)
    return 3;

  if (t1 * t2 > 0 && t1 * t3 > 0)
    return 0;
  else
    return -1;
}

// A point P lies inside a ccw triangle ABC iff P lies to the left of lines AB, BC, and CA.
bool point_in_tri(fvec2 v0, fvec2 v1, fvec2 v2, fvec2 point)
{
  // TODO: Replace with method from the real-time collision detection book.
  f32 float_error = 0.000001;
  // Remember this is in YZ plane
  fvec2 pa = fvec2_sub(point, v0);
  fvec2 pb = fvec2_sub(point, v1);
  fvec2 pc = fvec2_sub(point, v2);
  // PA and PB
  f32 t1 = fabs(cross2(pa, pb));
  f32 papb_y = pa.x * pb.x;
  f32 papb_z = pa.y * pb.y;
  bool overlapping1 = (t1 < float_error) && (papb_y <= 0) && (papb_z <= 0);
  if (overlapping1) return 1;
  // PB and PC
  f32 t2 = fabs(cross2(pb, pc));
  f32 pbpc_y = pb.x * pc.x;
  f32 pbpc_z = pb.y * pc.y;
  bool overlapping2 = (t2 < float_error) && (pbpc_y <= 0) && (pbpc_z <= 0);
  if (overlapping2) return 2;
  // PC and PA
  f32 t3 = fabs(cross2(pc, pa));
  f32 pcpa_y = pc.x * pa.x;
  f32 pcpa_z = pc.y * pa.y;
  bool overlapping3 = (t3 < float_error) && (pcpa_y <= 0) && (pcpa_z <= 0);
  if (overlapping3) return 3;
  if ((t1 * t2 > 0) && (t1 * t3 > 0))
  {
    return 0;
  }
  else
  {
    return -1;
  }
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
  // TODO: This is not actually the number of unique verts. I'm just saving each vert as a new element, but there are repeats.
  // TODO: I should only save the number of unique verts and determine how to get the index of the one I need. (hash map)?
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
  f32 x_scale = 1.0f / counts.x;
  f32 y_scale = 1.0f / counts.y;
  f32 z_scale = 1.0f / counts.z;
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


voxel_grid model_voxelize(mesh model, u32 resolution, arena *vert_buffer, arena *elem_buffer, arena *memory)
{
  voxel_grid grid = {};
  grid.min = model_min(model);
  grid.max = model_max(model);
  // Create an array that contains the enabled voxels
  u32 count = resolution * resolution * resolution;
  grid.contents = arena_push_array(memory, count, u8);
  memset(grid.contents, 0, count*sizeof(grid.contents[0]));
  return grid;
}


voxel_grid model_voxelize_solid(mesh model, u32 resolution, arena *vert_buffer, arena *elem_buffer, arena *memory)
{
  // Initialize output
  voxel_grid grid = {};
  // First create a bbox that is a cube of the maximum distance of the raw bbox.
  fvec3 min = model_min(model);
  fvec3 max = model_max(model);
  // Output initialization
  grid.min = min;
  grid.max = max;
  fvec3 lengths = fvec3_sub(max, min);
  f32 max_length = fvec3_max_elem(lengths);
  if (max_length != lengths.x)
  {
    f32 delta = max_length - lengths.x; // compute differences between largest length and current length.
    f32 padding = delta / 2.0f; // Half of the total padding.
    grid.min.x = min.x - padding; // Apply padding before model min.
    grid.max.x = max.x + padding; // Apply padding after model max.
  }
  if (max_length != lengths.y)
  {
    f32 delta = max_length - lengths.y; // compute differences between largest length and current length.
    f32 padding = delta / 2.0f; // Half of the total padding.
    grid.min.y = min.y - padding; // Apply padding before model min.
    grid.max.y = max.y + padding; // Apply padding after model max.
  }
  if (max_length != lengths.z)
  {
    f32 delta = max_length - lengths.z; // compute differences between largest length and current length.
    f32 padding = delta / 2.0f; // Half of the total padding.
    grid.min.z = min.z - padding; // Apply padding before model min.
    grid.max.z = max.z + padding; // Apply padding after model max.
  }
  // TODO: Is this the best fix?
  // In case a triangle is axis-aligned and lies on a voxel edge, it may or may not be counted.
  f32 offset = (1 / 10001.0f);
  fvec3 epsilon = fvec3_scale(fvec3_sub(grid.max, grid.min), offset);
  grid.min = fvec3_sub(grid.min, epsilon);
  grid.max = fvec3_add(grid.max, epsilon);
  
  // Calculate voxel units
  fvec3 bbox_diff = fvec3_sub(grid.max, grid.min);
  f32 bbox_divisor = (1.0f/resolution);
  fvec3 units = fvec3_scale(bbox_diff, bbox_divisor);
  // Set all verts of the model to its cube bbox min
  // vertex *verts_new = arena_push_array(memory, model.vert_count, vertex);
  // the cubed bbox_min
  // TODO: Should you update the model or make a copy?
  for (int i = 0; i < model.vert_count; ++i)
  {
    model.vertices[i].pos = fvec3_sub(model.vertices[i].pos, grid.min);
  }
  // Create mesh
  grid.max = fvec3_sub(grid.max, grid.min);
  grid.min = fvec3_sub(grid.min, grid.min);
  // Start voxelization. 
  // Create an array that contains the enabled voxels
  u32 count = resolution * resolution * resolution;
  grid.contents = arena_push_array(memory, count, u8);
  memset(grid.contents, 0, count*sizeof(grid.contents[0]));
  // Loop for each triangle
  u64 tri_count = ceil(model.index_count / 3);
  for (i64 i = 0; i < model.index_count; i+=3)
  {
    // Triangle vertices
    fvec3 v0 = model.vertices[model.indices[i+0]].pos;
    fvec3 v1 = model.vertices[model.indices[i+1]].pos;
    fvec3 v2 = model.vertices[model.indices[i+2]].pos;
    // Triangle edges
    fvec3 e0 = fvec3_sub(v1, v0);
    fvec3 e1 = fvec3_sub(v2, v1);
    fvec3 e2 = fvec3_sub(v0, v2);
    // Normal vector for triangle
    fvec3 norm = normalize3(cross3(e0, e1));
    if (fabs(norm.x) < 0.000001f) continue;
    // Project points into yz plane
    fvec2 v0_yz = fvec2{ {v0.y, v0.z} };
    fvec2 v1_yz = fvec2{ {v1.y, v1.z} };
    fvec2 v2_yz = fvec2{ {v2.y, v2.z} };
    // Ensure the triangle is winding counterclockwise
    bool is_ccw = 0;
    is_ccw = triangle_is_ccw(v0_yz, v1_yz, v2_yz);
    if (is_ccw == false)
    {
      // Its clockwise, fix it.
      fvec2 v3 = v1_yz;
      v1_yz = v2_yz;
      v2_yz = v3;
    }
    // Compute triangle bbox in grid
    fvec2 tri_bbox_min = fvec2_min(v0_yz, fvec2_min(v1_yz, v2_yz));
    fvec2 tri_bbox_max = fvec2_max(v0_yz, fvec2_max(v1_yz, v2_yz));
    // Convert world coordinates to grid coordinates
    // Dividing by units to get grid position, subtracting 0.5 to center the point.
    f32 min_y = ceil(tri_bbox_min.x / units.y - 0.5f);
    f32 min_z = ceil(tri_bbox_min.y / units.z - 0.5f);
    f32 max_y = floor(tri_bbox_max.x / units.y - 0.5f);
    f32 max_z = floor(tri_bbox_max.y / units.z - 0.5f);

    // Skip if bounding box is invalid (min > max means no voxel centers overlap)
    if (min_y > max_y || min_z > max_z) continue;

    fvec2 grid_min = fvec2{ {min_y, min_z} };
    fvec2 grid_max = fvec2{ {max_y, max_z} };
    // Determine indexing strides
    u32 row_stride = resolution;
    u32 slice_stride = resolution * resolution;
    // For each overlapping voxel examine YZ plane
    for (u32 y = grid_min.x; y <= (u32)ceil(grid_max.x); ++y)
    {
      for (u32 z = grid_min.y; z <= (u32)ceil(grid_max.y); ++z)
      {
        // 1. Check the location of the point and the triangle
        fvec2 point = fvec2{{ 
          ((y + 0.5f) * units.y), 
          ((z + 0.5f) * units.z)
        }};
        // Translate triangle so that point is origin
        u32 is_colliding = check_point_triangle(v0_yz, v1_yz, v2_yz, point);
        // 2. Check if point is inside, or touching an edge.
        if (
          ( (is_colliding == 1) && top_left_edge(v0_yz, v1_yz) ) ||
          ( (is_colliding == 2) && top_left_edge(v1_yz, v2_yz) ) ||
          ( (is_colliding == 3) && top_left_edge(v2_yz, v0_yz) ) ||
            (is_colliding == 0)
        )
        {
          // 3. Get X coordinate of the voxel
          u32 xmax = (u32) floor( voxel_x_get(norm, v0, point) / units.x );
          for (u32 x = 0; x <= xmax; ++x)
          {
            u32 location = x + (y * row_stride) + (z * slice_stride);
            grid.contents[location] ^= 1;
          }
        }
      }
    }
  }
  return grid;
}
