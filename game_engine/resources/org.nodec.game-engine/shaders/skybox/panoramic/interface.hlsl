#include <common/interface_scene.hlsl>

struct V2P {
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD0;
};

// struct MaterialProperties {

// };

Texture2D texMain : register(t0);
SamplerState sampler_texMain : register(s0);
