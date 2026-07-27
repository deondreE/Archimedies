#pragma pack_matrix(row_major)

cbuffer SceneBuffer : register(b0)
{
    float4x4 u_ViewProjection;
    float3 u_LightDirection;
    float _Pad0;
    float3 u_LightColor;
    float u_LightIntensity;
    float u_AmbientStrength;
    float3 _Pad1;
};

cbuffer EntityBuffer : register(b1)
{
    float4x4 u_World;
//    float4x4 u_NormalMatrix;
};

Texture2D u_Texture : register(t0);
SamplerState u_Sampler : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
    float3 worldNormal : NORMAL;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    float4 worldPos = mul(float4(input.pos, 1.0f), u_World);
    output.pos = mul(worldPos, u_ViewProjection);
    
    output.worldNormal = mul(input.normal, (float3x3) u_World);
    output.col = input.col;
    output.uv = input.uv;
    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
    float3 normal = normalize(input.worldNormal);
    
    float3 toLight = normalize(-u_LightDirection);
    
    float diffuseFactor = max(dot(normal, toLight), 0.0f);
    float3 diffuse = u_LightColor * u_LightIntensity * diffuseFactor;
    float3 ambient = u_LightColor * u_AmbientStrength;
    
    float3 lighting = ambient + diffuse;
    
    float4 texColor = u_Texture.Sample(u_Sampler, input.uv);
    float4 baseColor = texColor * input.col;
    
    return float4(baseColor.rgb * lighting, baseColor.a);
}