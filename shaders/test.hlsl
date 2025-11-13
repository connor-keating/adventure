
struct VSIn {
  float3 pos   : POSITION;
  float4 color : COLOR0;
  float2 uv    : TEXCOORD0;
};

struct VSOut {
  float4 pos   : SV_POSITION;
  float4 color : COLOR0;
  float2 uv    : TEXCOORD0;
};

VSOut VSMain(VSIn i)
{ 
  /*
  // vid=0; float4(0.0, 0.0, 1, 1.5)-0.5=(-0.5, -0.5, 0.5, 1.0), red (1,0,0,1)
  // vid=1; float4(0.5, 1.0, 1, 1.5)-0.5=( 0.0,  0.5, 0.5, 1.0), grn (0,1,0,1)
  // vid=2; float4(1.0, 0.0, 1, 1.5)-0.5=( 0.5, -0.5, 0.5, 1.0), blu (0,0,1,1)
  VSOut output = { 
    float4(vid * 0.5f, vid & 1, 1, 1.5f) - 0.5f,
    float4(vid == 0, vid == 1, vid == 2, 1),
    float2(0.0f, 0.0f)
  };
  */
  VSOut output = {
    float4(i.pos, 1.0f),
    i.color,
    float2(0.0f, 0.0f)
  };
  return output;
}

float4 PSMain(VSOut i) : SV_Target
{
  return i.color;
}