#include "../../common/interface_scene.hlsl"

struct MaterialProperties {
    float4 color;
    float4 near_far;
};

cbuffer cbMaterialProperties : register(b3) {
    MaterialProperties materialProperties;
};

// --- each pass properties ---
Texture2D texScreen : register(t0);
Texture2D texDepth : register(t1);
SamplerState sampler_tex : register(s0);
// ---

struct V2P {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};
