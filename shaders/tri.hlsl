
struct out_vertex { float4 pos : SV_POSITION; float4 col : COL; };

out_vertex VSMain(uint vid : SV_VERTEXID)
{ 
  // if CW:
  // float4(vid * 0.5f, vid & 1, 1, 1.5f) - 0.5f;
    // vid=0; float4(0.0, 0.0, 1, 1.5)-0.5=(-0.5, -0.5, 0.5, 1.0), red (1,0,0,1)
    // vid=1; float4(0.5, 1.0, 1, 1.5)-0.5=( 0.0,  0.5, 0.5, 1.0), grn (0,1,0,1)
    // vid=2; float4(1.0, 0.0, 1, 1.5)-0.5=( 0.5, -0.5, 0.5, 1.0), blu (0,0,1,1)
  // if CCW:
  float xcomp = vid * -0.5f;
  float ycomp = ((vid & 1)==0) * -1.0f;
  float4 pos = float4( xcomp , ycomp, 0.0f, 0.5f ) + 0.5f;
  out_vertex output = { 
    pos,
    float4(vid == 0, vid == 1, vid == 2, 1) 
  };
  return output;
}

float4 PSMain(out_vertex input) : SV_TARGET 
{ 
  return input.col;
}