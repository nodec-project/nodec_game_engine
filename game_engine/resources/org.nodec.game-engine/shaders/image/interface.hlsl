// --- Image Shader Interface ---

#include <common/interface_model.hlsl>

struct V2P {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 position_world : POSITION;
    float3 normal_world : NORMAL;
    float4 depth : TEXCOORD1;
};

struct MaterialProperties {
    float4 color;
};

cbuffer cbMaterialProperties : register(b3)
{
    MaterialProperties materialProperties;
};

Texture2D texImage : register(t0);
Texture2D texMask : register(t1);

SamplerState sampler_texImage : register(s0);
SamplerState sampler_texMask : register(s1);
