
struct out_vertex { float4 pos : SV_POSITION; float4 col : COL; };

out_vertex VSMain(uint vid : SV_VERTEXID)
{ 
  // vid=0; float4(0.0, 0.0, 1, 1.5)-0.5=(-0.5, -0.5, 0.5, 1.0), red (1,0,0,1)
  // vid=1; float4(0.5, 1.0, 1, 1.5)-0.5=( 0.0,  0.5, 0.5, 1.0), grn (0,1,0,1)
  // vid=2; float4(1.0, 0.0, 1, 1.5)-0.5=( 0.5, -0.5, 0.5, 1.0), blu (0,0,1,1)
  out_vertex output = { 
    float4(vid * 0.5f, vid & 1, 1, 1.5f) - 0.5f,
    float4(vid == 0, vid == 1, vid == 2, 1) 
  };
  return output;
}

float4 PSMain(out_vertex input) : SV_TARGET 
{ 
  return input.col;
}