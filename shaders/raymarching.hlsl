struct VSIn {
  float3 pos   : POSITION;
  float4 color : COLOR0;
  float2 uv    : TEXCOORD0;
};

struct VSOut {
  float4 pos   : SV_POSITION;
  float4 color : COLOR0;
  float2 uv    : TEXCOORD0;
};

Texture3D<float> voxelTexture : register(t0);
SamplerState voxelSampler : register(s0);

cbuffer camera : register(b0)
{
  float4x4 view_inv;            // 64 bytes
  float4x4 proj_inv;            // 64 bytes
  float4x4 wrld_inv; // 64 bytes - inverse rotation for volume
  float3 camera_pos;            // 12 bytes
  float _padding;               // 4 bytes
};

// Ray-AABB (Axis-Aligned Bounding Box) intersection
// Returns true if ray intersects box, and outputs t_near and t_far
bool ray_box_intersection(
  float3 ray_origin,
  float3 ray_dir,
  float3 box_min,
  float3 box_max,
  out float t_near,
  out float t_far)
{
  // Calculate intersection distances for each axis
  float3 t1 = (box_min - ray_origin) / ray_dir;
  float3 t2 = (box_max - ray_origin) / ray_dir;

  // Find the near and far intersection points for each slab
  float3 t_min = min(t1, t2);
  float3 t_max = max(t1, t2);

  // The ray enters the box at the latest entry point
  t_near = max(max(t_min.x, t_min.y), t_min.z);

  // The ray exits the box at the earliest exit point
  t_far = min(min(t_max.x, t_max.y), t_max.z);

  // Ray intersects if t_near < t_far and t_far > 0
  return t_near < t_far && t_far > 0.0f;
}

VSOut VSMain(VSIn i)
{
  VSOut output = {
    // mul(view_projection, float4(i.pos, 1.0f)),
    float4(i.pos, 1.0f),
    i.color,
    i.uv
  };
  return output;
}

float4 PSMain(VSOut i) : SV_Target
{
  // Step 1: Convert UV (0,1) to NDC (-1,1)
  float2 uv = i.uv;              // 0..1
  float2 ndc = uv * 2.0f - 1.0f; //-1..1

  // Step 2: Generate world-space ray using inverse view-projection
  // Create a point on the far plane in clip space (z=1 for D3D depth range [0,1])
  float4 clip_space = float4(ndc.x, ndc.y, 1.0f, 1.0f);

  // Transform to view space using inverse projection
  float4 view_space = mul(proj_inv, clip_space);
  view_space /= view_space.w;  // Perspective divide

  // Transform to world space using inverse view
  float3 world_pos = mul(view_inv, float4(view_space.xyz, 1.0f)).xyz;

  // Ray direction from camera to world position
  float3 ray_dir = normalize(world_pos - camera_pos);

  // Step 3: Transform ray to volume's local space (apply inverse rotation)
  // This allows us to rotate the volume while keeping AABB intersection simple
  float3 ray_origin_local = mul(wrld_inv, float4(camera_pos, 1.0f)).xyz;
  float3 ray_dir_local = mul(wrld_inv, float4(ray_dir, 0.0f)).xyz;
  ray_dir_local = normalize(ray_dir_local);

  // Step 4: Define volume bounding box
  // Our 3x3x3 voxel grid occupies a 1x1x1 cube centered at origin
  // float3 color = ray_dir * 0.5f + 0.5f;
  float3 box_min = float3(-0.5f, -0.5f, -0.5f);
  float3 box_max = float3( 0.5f,  0.5f,  0.5f);

  // Step 5: Test ray-box intersection (using local-space ray)
  float t_near, t_far;
  bool hit = ray_box_intersection(ray_origin_local, ray_dir_local, box_min, box_max, t_near, t_far);

  // Step 6: Raymarch through the volume
  float3 color;
  if (hit)
  {
    // Ensure we start in front of the camera
    t_near = max(t_near, 0.0f);

    // Raymarching parameters
    float step_size = 0.01f;  // Small steps for quality (1/100th of a unit)
    int max_steps = 300;      // Safety limit to prevent infinite loops

    // Opacity control: how opaque each sample is
    // What percentage opacity does each sample contribute? (5% = .05)
    float opacity_factor = 0.05f;  // Lower = more transparent, higher = more opaque

    // Initialize accumulators for front-to-back compositing
    float3 color_accum = float3(0.0f, 0.0f, 0.0f);  // Accumulated color
    float alpha_accum = 0.0f;                        // Accumulated opacity

    // March along the ray (front-to-back)
    float t = t_near;
    for (int step = 0; step < max_steps && t < t_far; ++step)
    {
      // Early termination: stop if ray is fully opaque
      if (alpha_accum > 0.99f)
        break;

      // Calculate current position in volume's local space
      float3 local_pos = ray_origin_local + t * ray_dir_local;

      // Convert local space [-0.5, 0.5] to texture space [0, 1]
      float3 tex_coord = local_pos + 0.5f;

      // Sample the 3D texture
      // Use SampleLevel to avoid gradient instruction warning in loops
      // Level 0 = highest resolution mipmap
      float density = voxelTexture.SampleLevel(voxelSampler, tex_coord, 0);

      // If we hit a non-zero voxel, composite it
      if (density > 0.0f)
      {
        // Calculate opacity for this sample
        float alpha = density * opacity_factor;

        // For now, use white color (we'll add transfer functions later)
        float3 sample_color = float3(1.0f, 1.0f, 1.0f);

        // Front-to-back compositing
        float weight = (1.0f - alpha_accum) * alpha;
        color_accum += weight * sample_color;
        alpha_accum += weight;
      }

      // Step forward along the ray
      t += step_size;
    }

    // Final color with background blending
    color = color_accum;
  }
  else
  {
    // Ray misses the volume - show background color
    color = float3(0.1f, 0.1f, 0.2f); // Dark blue background
  }

  return float4(color, 1.0f);
}