#include "combine_interface.hlsl"

float4 PSMain(V2P input) : SV_TARGET {
    float4 base_color = tex_screen.Sample(sampler_tex, input.texcoord);
    float4 reflection_color = tex_reflection_color.Sample(sampler_tex, input.texcoord);

    // const float3 normal_in_world_space = tex_normal.Sample(sampler_tex, input.texcoord);
    // return float4(normal_in_world_space, 1);
    

    // return lerp(float4(0,0 ,0, 0), reflection_color, reflection_color.a);
    // return reflection_color;

    // return float4(base_color.rgb, 1);
    // return base_color;
    // return reflection_color;
    // return float4(reflection_color.r, 0, 0, 1); 

    float4 color = base_color;
    color.rgb = lerp(base_color.rgb, reflection_color.rgb, clamp(reflection_color.a, 0, 1));
    return float4(color.rgb, 1);
}
