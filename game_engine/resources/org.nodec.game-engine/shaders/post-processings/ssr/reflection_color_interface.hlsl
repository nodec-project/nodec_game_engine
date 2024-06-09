#include "interface.hlsl"

Texture2D tex_reflection_uv : register(t0);
Texture2D tex_screen : register(t1);
SamplerState sampler_tex : register(s0);

struct V2P {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};