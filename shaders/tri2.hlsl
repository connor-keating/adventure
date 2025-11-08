// Each vertex = float3 position (x,y,z) + float3 color (r,g,b)

// Input structure: must match D3D11_INPUT_ELEMENT_DESC in C++
struct VSInput
{
    float3 pos : POSITION;   // 12 bytes offset 0
    float4 col : COLOR;      // 16 bytes offset 12
    float2 tex : TEXCOORD;   // 8  bytes offset 28
};

// Vertex shader output: position goes to SV_POSITION, color passed to pixel shader
struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 tex : TEXCOORD;
};

Texture2D tex_diffuse : register(t0);
SamplerState tex_sampler : register(s0);

// Vertex Shader
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // Convert to homogeneous clip-space position
    // (here we assume input is already in clip space for simplicity)
    output.pos = float4(input.pos, 1.0f);

    // Pass color through
    output.col = input.col;

    // Pass texture coordinates through
    output.tex = input.tex;

    return output;
}

// Pixel Shader
float4 PSMain(VSOutput input) : SV_TARGET
{
    // Sample texture and multiply by vertex color
    float4 tex_color = tex_diffuse.Sample(tex_sampler, input.tex);
    return tex_color * input.col;
}
