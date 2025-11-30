
struct vertex { float4 pos : SV_POSITION; float4 col : COL; };

cbuffer cam : register(b0)
{
  float4x4 view_projection;
};

vertex VSMain(uint vid : SV_VERTEXID)
{ 
  const float4x4 ortho = float4x4(
    0.2f, 0.0f, 0.0f, 0.0f, // row 0
    0.0f, 0.2f, 0.0f, 0.0f, // row 1
    0.0f, 0.0f, 0.5f, 0.0f, // row 2
    0.0f, 0.0f, 0.5f, 1.0f  // row 3
  );
  float4 pos = float4(vid * 0.5f, vid & 1, 1, 1.5f) - 0.5f;
  float4 col = float4(vid == 0, vid == 1, vid == 2, 1);
  // float4 final_pos = mul(ortho, pos);
  float4 final_pos = mul(view_projection, pos);
  vertex output = { 
    final_pos,
    col
  };
  return output;
}

float4 PSMain(vertex input) : SV_TARGET 
{ 
  return input.col;
}