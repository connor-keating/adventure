// Each vertex = float3 position (x,y,z) + float3 color (r,g,b)

// Input structure: must match D3D11_INPUT_ELEMENT_DESC in C++
struct VSInput
{
    float3 pos : POSITION;   // 12 bytes offset 0
    float3 col : COLOR;      // 12 bytes offset 12
};

// Vertex shader output: position goes to SV_POSITION, color passed to pixel shader
struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

// Vertex Shader
VSOutput vertex_shader(VSInput input)
{
    VSOutput output;

    // Convert to homogeneous clip-space position
    // (here we assume input is already in clip space for simplicity)
    output.pos = float4(input.pos, 1.0f);

    // Pass color through
    output.col = input.col;

    return output;
}

// Pixel Shader
float4 pixel_shader(VSOutput input) : SV_TARGET
{
    // Just output the vertex color as final pixel color
    return float4(input.col, 1.0f);
}
