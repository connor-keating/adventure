
struct vertex_in  { float3 pos : POSITION0; float4 col : COLOR0; float2 texcoord : TEXCOORD0; };
struct vertex_out { float4 pos : SV_POSITION; float4 col : COLOR; float2 texcoord : TEXCOORD0; };

cbuffer camera : register(b0)
{
  float4x4 view;
  float4x4 proj;
  float3   pos;
  float    _pad;
};

cbuffer transform : register(b1)
{
  float4x4 world;
};

Texture2D mainTexture : register(t0);
SamplerState mainSampler : register(s0);

vertex_out VSMain( vertex_in input )
{
  float4 world_position = mul(world, float4(input.pos, 1.0f));
  float4 out_position = mul(proj, mul(view, world_position) );
  vertex_out output = {
    out_position,
    input.col,
    input.texcoord
  };
  return output;
}

float4 PSMain(vertex_out input) : SV_TARGET
{
  float4 texColor = mainTexture.Sample(mainSampler, input.texcoord);
  // If color alpha is 0, use raw texture; otherwise blend with color
  return (input.col.a == 0.0f) ? texColor : texColor * input.col;
}