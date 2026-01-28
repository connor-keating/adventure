
#define PI 	3.14159265358979323846f

struct vertex_in  { float3 pos : POSITION0; float4 col : COLOR0; float2 texcoord : TEXCOORD0; };
struct vertex_out { float4 pos : SV_POSITION; float4 col : COLOR; float2 texcoord : TEXCOORD0; };

cbuffer camera : register(b0)
{
  float4x4 view;
  float4x4 proj;
  float3   pos;
  float    dt;
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
  float2 uv = input.texcoord;

  // Two colors to blend between
  float4 color1 = float4(0.2f, 0.0f, 0.8f, 0.0f);  // purple
  float4 color2 = float4(0.0f, 0.8f, 0.8f, 1.0f);  // cyan

  // Sin wave along Y axis (frequency controls number of bands)
  float frequency = 5.0f;
  float speed = 0.5f;
  float wave = sin((uv.y - dt * speed) * frequency * PI);

  // Map from [-1, 1] to [0, 1] for blending
  float blend = wave * 0.5f + 0.5f;

  // Lerp between the two colors
  float4 color = lerp(color1, color2, blend);
  return color;
}