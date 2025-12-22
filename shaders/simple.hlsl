
struct vertex_in
{ 
  float4 pos : POSITION0;
  float4 col : COLOR0;
  uint id: SV_InstanceID;
};

struct vertex_out 
{ 
  float4 pos : SV_POSITION;
  float4 col : COL;
};

vertex_out VSMain(vertex_in input)
{
  vertex_out o;
  o.pos = input.pos + float4(input.id, 0.0f, 0.0f, 0.0f);
  o.col = input.col;
  return o;
}

float4 PSMain(vertex_out input) : SV_TARGET
{
  return input.col;
}