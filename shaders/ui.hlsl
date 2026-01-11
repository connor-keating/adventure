struct in_vertex 
{
  uint vid: SV_VERTEXID;
  float4 color: COLOR0;
};


struct out_vertex 
{ 
  float4 pos : SV_POSITION;
  float4 col : COL;
};

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

out_vertex VSMain(in_vertex input)
{
  float4 pos = mul( proj, float4(square[input.vid], 0.0f, 1.0f) );
  out_vertex output = {
    pos,
    input.color
  };
  return output;
}

float4 PSMain(out_vertex input) : SV_TARGET 
{ 
  return input.col;
}