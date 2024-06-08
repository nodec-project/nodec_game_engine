#include "reflection_color_interface.hlsl"

struct PSOut {
    float4 color : SV_TARGET0;
};

PSOut PSMain(V2P input) {
    PSOut output;

    float screen_tex_width, screen_tex_height;
    tex_screen.GetDimensions(screen_tex_width, screen_tex_height);

    float4 reflection_uv = tex_reflection_uv.Sample(sampler_tex, input.texcoord);
    // return float4(reflection_uv.xy, 0, 1);
    if (reflection_uv.b <= 0.0) {
        output.color = float4(0, 0, 0, 1);
        return output;
    }
    // float4 color = tex_screen.Sample(sampler_tex, input.texcoord);
    float4 color = tex_screen.Sample(sampler_tex, reflection_uv);
    float alpha = clamp(reflection_uv.b, 0, 1);
    // alpha = 1;

    // return float4(lerp(float3(0,0,0), color.rgb, alpha), 1);
    
    output.color = float4(lerp(float3(0,0,0), color.rgb, alpha), 1);
    return output;
    // return float4(color.rgb, 1);
    // return float4(reflection_uv.xy, 0, 1);
}