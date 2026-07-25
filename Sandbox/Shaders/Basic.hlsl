#pragma pack_matrix(row_major)

cbuffer WVPBuffer : register(b0)
{
    float4x4 u_WVP;
};

Texture2D u_Texture : register(t0);
SamplerState u_Sampler : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    output.pos = mul(float4(input.pos, 1.0f), u_WVP);
    output.col = input.col;
    output.uv = input.uv;
    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
    float4 texColor = u_Texture.Sample(u_Sampler, input.uv);
    return texColor * input.col; // tint texture by vertex color; drop "* input.col" if you don't want tinting
}