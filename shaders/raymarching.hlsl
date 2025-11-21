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
  // Get NDC (-1,1) from UV (0,1) coords 
  float2 uv = i.uv;              // 0..1
  float2 ndc = uv * 2.0f - 1.0f; //-1..1
  float4 final_color = float4(uv, 0.0f, 1.0f);
  return final_color;
}