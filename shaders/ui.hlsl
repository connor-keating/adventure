
struct vertex_in  { float3 pos : POSITION0; float4 col : COLOR0; };
struct vertex_out { float4 pos : SV_POSITION; float4 col : COLOR; };

cbuffer camera : register(b0)
{
  float4x4 view;
  float4x4 proj;
  float3   pos;
  float    _pad;
};


vertex_out VSMain( vertex_in input )
{ 
  float4 out_position = mul(proj, mul(view, float4(input.pos, 1.0f) ));
  vertex_out output = { 
    out_position,
    input.col
  };
  return output;
}

float4 PSMain(vertex_out input) : SV_TARGET 
{ 
  return input.col;
}