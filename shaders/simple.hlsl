
// Data types
struct vertex_in
{ 
  float4     pos: POSITION0;
  float2     tex: TEXCOORD0;
  float4     col: COLOR0;
  float4x4 world: WORLD;
  float      hot: HOT;
  uint id: SV_InstanceID;
};

struct vertex_out
{
  float4 pos : SV_POSITION;
  float4 col : COLOR0;
  float2 pos_local : TEXCOORD0;
  float  hot : HOT;
};

// Constant buffers
cbuffer cam : register(b0)
{
  float4x4 view;
  float4x4 proj;
};


vertex_out VSMain(vertex_in input)
{
  vertex_out o;
  o.pos = mul(proj, mul(input.world, input.pos));
  o.col = input.col;
  // Remap range of tex to (-1, 1) in the box
  o.pos_local = input.tex * 2.0f - 1.0f;
  o.hot = input.hot;
  return o;
}


float4 PSMain(vertex_out input) : SV_TARGET
{
  // Input.pos_local ranges from -1 to 1
  float2 p = input.pos_local;

  // Box size (leaving room for rounded corners)
  float2 boxSize = float2(1.0, 1.0);

  // Corner radius (adjust this to change roundness)
  float cornerRadius = 0.4;

  // Border thickness (adjust to make border thicker/thinner)
  float borderThickness = 0.1;

  // Calculate signed distance field for rounded box
  float2 d = abs(p) - boxSize + cornerRadius;
  float dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - cornerRadius;

  // Discard pixels outside the rounded box
  if (dist > 0.0) discard;

  // Calculate inner distance for border
  float innerDist = dist + borderThickness;

  // Determine if we're in the border region
  float4 final_color = input.col;
  if (innerDist > 0.0)
  {
    // We're in the border region - use white
    final_color = float4(1.0, 1.0, 1.0, input.hot);
  }

  return final_color;
}