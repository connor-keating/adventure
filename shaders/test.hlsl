
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

Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

VSOut VSMain(VSIn i)
{
  VSOut output = {
    float4(i.pos, 1.0f),
    i.color,
    i.uv
  };
  return output;
}

float4 PSMain(VSOut i) : SV_Target
{
  float alpha = fontTexture.Sample(fontSampler, i.uv).r;
  return float4(i.color.rgb, i.color.a * alpha);
}