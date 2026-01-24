struct vertex_in
{ 
  float3 pos : POSITION0;
  float4 col : COLOR0;
  float2 texcoord : TEXCOORD0;
};

struct vertex_out
{ 
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 world_pos : WORLD0;
};

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

float4 grid_local(float cell_count, float line_width, float2 pos)
{
  // Constants
  float4 line_thick_color = float4(1.0f, 1.0f, 1.0f, 1.0f);
  float4 line_thin_color = float4(0.5f, 0.5f, 0.5f, 1.0f);
  float4 cell_color = float4(0.0f, 0.0f, 0.0f, 1.0f);

  // UV goes 0-1, scale to number of cells
  float2 grid_pos = pos * cell_count;
  float2 grid_dist = abs(frac(grid_pos) - 0.5);
  float2 line_dist = 0.5 - grid_dist;
  float half_width = line_width * cell_count * 0.5;

  // Thin lines for all cells
  float4 color = cell_color;
  if (line_dist.x < half_width || line_dist.y < half_width)
  {
    color = line_thin_color;
  }

  // Major lines every 2 cells
  float major_spacing = 5.0f;
  float2 major_grid_pos = grid_pos / major_spacing;
  float2 major_dist = abs(frac(major_grid_pos) - 0.5);
  float2 major_line_dist = 0.5 - major_dist;
  float major_half_width = (line_width * 2.0) * cell_count / major_spacing * 0.5;
  if (major_line_dist.x < major_half_width || major_line_dist.y < major_half_width)
  {
    color = line_thick_color;
  }
  return color;
}

float4 grid_world(float3 world_pos)
{
  // Constants
  float thin_width = 0.02f;   // Thin line thickness in world units
  float thick_width = 0.04f;  // Major line thickness in world units
  float major_spacing = 5.0f; // Major lines every N units
  float4 line_thick_color = float4(1.0f, 1.0f, 1.0f, 1.0f);
  float4 line_thin_color = float4(0.5f, 0.5f, 0.5f, 1.0f);
  float4 cell_color = float4(0.0f, 0.0f, 0.0f, 1.0f);

  // Distance to nearest grid line (1 unit spacing)
  float2 dist_to_line = abs(world_pos.xz - round(world_pos.xz));

  // Distance to nearest major grid line (N unit spacing)
  float2 major_pos = world_pos.xz / major_spacing;
  float2 dist_to_major = abs(major_pos - round(major_pos)) * major_spacing;

  // Determine color based on proximity to lines
  float4 color = cell_color;
  if (dist_to_line.x < thin_width || dist_to_line.y < thin_width)
  {
    color = line_thin_color;
  }
  if (dist_to_major.x < thick_width || dist_to_major.y < thick_width)
  {
    color = line_thick_color;
  }

  return color;
}

vertex_out VSMain( vertex_in input )
{
  float4 world_position = mul(world, float4(input.pos, 1.0f));
  float4 out_position = mul(proj, mul(view, world_position));
  vertex_out output = {
    out_position,
    input.texcoord,
    world_position.xyz
  };
  return output;
}

float4 PSMain(vertex_out input) : SV_TARGET
{
  float4 color = grid_world(input.world_pos);
  return color;
}