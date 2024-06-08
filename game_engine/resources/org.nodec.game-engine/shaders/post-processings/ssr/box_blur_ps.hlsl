#include "box_blur_interface.hlsl"

struct PSOut {
    float4 color : SV_Target0;
};

float4 PSMain(V2P input) : SV_TARGET {
    PSOut output;

    float tex_width, tex_height;
    tex_reflection_color.GetDimensions(tex_width, tex_height);

    float4 color = tex_reflection_color.Sample(sampler_tex, input.texcoord);
    color.a = 1; // ???
    
    // return color;
    
    int size = 1;
    if (size <= 0) {
        return color;
        // output.color = color;
        // return output;
    }

    int separation = 1;
    separation = max(separation, 1);

    color = float4(0, 0, 0, 1);
    float count = 0.0;

    for (int i = -size; i <= size; ++i) {
        for (int j = -size; j <= size; ++j) {
            color.rgb += tex_reflection_color.Sample(sampler_tex,
                input.texcoord + float2(i * separation / tex_width, j * separation / tex_height)).rgb;
            count += 1.0;
        }
    }

    color.rgb /= count;
    return color;
    // output.color = color;
    // return output;
}
