struct vertex_in  { float3 pos : POSITION0; float4 col : COLOR0; float2 texcoord : TEXCOORD0; };
struct vertex_out { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

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

vertex_out VSMain( vertex_in input )
{
  float4 world_position = mul(world, float4(input.pos, 1.0f));
  float4 out_position = mul(proj, mul(view, world_position));
  vertex_out output = {
    out_position,
    input.texcoord
  };
  return output;
}

float4 PSMain(vertex_out input) : SV_TARGET
{
  // Settings: 10x10 grid, major lines every 2 cells
  float grid_cells = 10.0f;
  float line_width = 0.02f;
  float4 line_thick_color = float4(1.0f, 1.0f, 1.0f, 1.0f);
  float4 line_thin_color = float4(0.5f, 0.5f, 0.5f, 1.0f);
  float4 cell_color = float4(0.0f, 0.0f, 0.0f, 1.0f);

  // UV goes 0-1, scale to number of cells
  float2 grid_pos = input.uv * grid_cells;
  float2 grid_dist = abs(frac(grid_pos) - 0.5);
  float2 line_dist = 0.5 - grid_dist;
  float half_width = line_width * grid_cells * 0.5;

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
  float major_half_width = (line_width * 2.0) * grid_cells / major_spacing * 0.5;
  if (major_line_dist.x < major_half_width || major_line_dist.y < major_half_width)
  {
    color = line_thick_color;
  }

  return color;
}