#include "combine_interface.hlsl"

float4 PSMain(V2P input) : SV_TARGET {
    float4 base_color = tex_screen.Sample(sampler_tex, input.texcoord);
    float4 reflection_color = tex_reflection_color.Sample(sampler_tex, input.texcoord);

    // return float4(base_color.rgb, 1);
    // return base_color;
    return reflection_color;
    // return float4(reflection_color.r, 0, 0, 1); 

    float4 color = base_color;
    color.rgb += reflection_color.rgb;
    return float4(color.rgb, 1);
}
