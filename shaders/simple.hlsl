
// Data types
struct vertex_in
{ 
  float4 pos :      POSITION0;
  float4 col :      COLOR0;
  float4x4 world:   WORLD;
  uint id: SV_InstanceID;
};

struct vertex_out
{
  float4 pos : SV_POSITION;
  float4 col : COL;
  float2 localPos : TEXCOORD0;
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
  // Pass local position (ranges from -1 to 1 in the box)
  o.localPos = input.pos.xy;
  return o;
}

float sdRoundedBox(float2 p, float2 size, float radius)
{
}

float4 PSMain(vertex_out input) : SV_TARGET
{
  // Input.localPos ranges from -1 to 1
  float2 p = input.localPos;

  // Box size (leaving room for rounded corners)
  float2 boxSize = float2(1.0, 1.0);

  // Corner radius (adjust this to change roundness)
  float cornerRadius = 0.4;

  // Calculate signed distance field for rounded box
  float2 d = abs(p) - boxSize + cornerRadius;
  float dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - cornerRadius;

  // Use the SDF to create smooth edges
  // smoothstep creates anti-aliased edge
  float alpha = 1.0 - smoothstep(-0.01, 0.01, dist);

  // Discard pixels outside the rounded box for sharp cutoff
  // Or use alpha for blending
  if (dist > 0.0) discard;

  return float4(input.col.xyz, alpha);
}