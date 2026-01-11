
struct vertex_in  { float3 pos : POSITION0; float4 col : COLOR0; };
struct vertex_out { float4 pos : SV_POSITION; float4 col : COLOR; };

vertex_out VSMain( vertex_in input )
{ 
  vertex_out output = { 
    float4(input.pos, 1.0f),
    input.col
  };
  return output;
}

float4 PSMain(vertex_out input) : SV_TARGET 
{ 
  return input.col;
}