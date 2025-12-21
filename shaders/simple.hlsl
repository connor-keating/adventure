
struct vertex_in
{ 
  float4 pos : POSITION;
};

struct vertex_out 
{ 
  float4 pos : SV_POSITION;
  float4 col : COL;
};

vertex_out VSMain(vertex_in input)
{
  vertex_out o;
  o.pos = input.pos;
  o.col = float4(1.0f, 1.0f, 1.0f, 1.0f);
  return o;
}

float4 PSMain(vertex_out input) : SV_TARGET
{
  return input.col;
}