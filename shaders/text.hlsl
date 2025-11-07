struct VSIn {
  float3 pos : POSITION;
  float2 uv  : TEXCOORD0;
};

struct VSOut {
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
};

VSOut VSMain(VSIn i)
{
  VSOut o;
  o.pos = float4(i.pos, 1.0); // assuming pos already in clip-space for minimalism
  o.uv  = i.uv;
  return o;
}

Texture2D    gTex  : register(t0);
SamplerState gSamp : register(s0);

float4 PSMain(VSOut i) : SV_Target
{
  return gTex.Sample(gSamp, i.uv);
}
