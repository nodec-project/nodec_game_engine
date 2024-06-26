#include "interface.hlsl"

Texture2D texDepth : register(t0);
Texture2D texNormal : register(t1);
Texture2D texScreen : register(t2);
SamplerState sampler_tex : register(s0);

struct V2P {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};