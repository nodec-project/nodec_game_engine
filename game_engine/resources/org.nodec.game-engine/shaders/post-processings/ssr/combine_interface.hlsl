#include "interface.hlsl"

Texture2D tex_screen : register(t0);
Texture2D tex_reflection_color : register(t1);
Texture2D tex_mat_props : register(t2);
Texture2D tex_normal : register(t3);
Texture2D tex_depth : register(t4);
SamplerState sampler_tex : register(s0);

struct V2P {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};