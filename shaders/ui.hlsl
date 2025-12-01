
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

Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);


cbuffer camera : register(b1)
{
  float4x4 view_projection;
};


VSOut VSMain(VSIn i)
{
  float4 pos_local = float4(i.pos, 1.0f);
  float4 pos_global = mul(view_projection, pos_local);
  // float4 pos_global = mul(pos_local, view_projection);
  VSOut output = {
    pos_global,
    i.color,
    i.uv
  };
  return output;
}

float4 PSMain(VSOut i) : SV_Target
{
  return float4(i.color.rgb, i.color.a);
}