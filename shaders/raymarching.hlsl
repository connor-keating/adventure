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
  float4x4 view_projection;
};

VSOut VSMain(VSIn i)
{
  VSOut output = {
    mul(view_projection, float4(i.pos, 1.0f)),
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

  // Step 2: Set up camera
  // Camera is at z=-2, looking at origin (0,0,0) down the +Z axis
  float3 camera_pos = float3(0.0f, 0.0f, -2.0f);

  // Step 3: Calculate ray direction
  // Since we have no projection, we map NDC directly to a plane at z=0
  // The ray goes from camera through this point on the view plane
  float3 point_on_viewplane = float3(ndc.x, ndc.y, 0.0f);
  float3 ray_dir = normalize(point_on_viewplane - camera_pos);

  // Step 4: Visualize ray direction as color
  // Map direction components from (-1,1) to (0,1) for RGB visualization
  float3 color = ray_dir * 0.5f + 0.5f;

  return float4(color, 1.0f);
}