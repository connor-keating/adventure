struct out_vertex { float4 pos : SV_POSITION; float4 col : COL; };

cbuffer camera : register(b0)
{
  float4x4 view;
  float4x4 proj;
  float3   pos;
  float    _pad;
};

static const float2 square[6] = {
  float2(-1.0f, -1.0f),  // Triangle 1
  float2( 1.0f, -1.0f),
  float2( 1.0f,  1.0f),
  float2( 1.0f,  1.0f),  // Triangle 2
  float2(-1.0f,  1.0f),
  float2(-1.0f, -1.0f)
};
out_vertex VSMain(uint vid : SV_VERTEXID)
{
  float4 pos = mul( proj, float4(square[vid], 0.0f, 1.0f) );
  out_vertex output = { 
    pos,
    float4(1.0f, 1.0f, 1.0f, 1.0f)
  };
  return output;
}

float4 PSMain(out_vertex input) : SV_TARGET 
{ 
  return input.col;
}