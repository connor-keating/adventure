
Texture2D    gTex  : register(t1);
SamplerState gSamp : register(s1);

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
  VSOut o;
  o.pos = float4(i.pos, 1.0);
  o.color = i.color;
  o.uv  = i.uv;
  return o;
}

float4 PSMain(VSOut i) : SV_Target
{
  // Sample the grayscale font atlas (1-channel texture)
  float alpha = gTex.Sample(gSamp, i.uv).r;

  // Return solid vertex color where texture is solid
  float4 final = float4(i.color.rgb, i.color.a * alpha);
  return final;
}
