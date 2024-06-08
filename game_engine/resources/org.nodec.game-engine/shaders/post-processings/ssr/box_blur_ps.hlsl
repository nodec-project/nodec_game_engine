#include "box_blur_interface.hlsl"

struct PSOut {
    float4 color : SV_Target0;
};

PSOut PSMain(V2P input) {
    PSOut output;
    float4 color = tex_reflection_color.Sample(sampler_tex, input.texcoord);
    
    int size = 1;
    if (size <= 0) {
        output.color = color;
        return output;
    }

    int separation = 2;
    separation = max(separation, 1);

    float count = 0.0;

    for (int i = -size; i <- size; ++i) {
        for (int j = -size; j <= size; ++j) {
            color.rgb += tex_reflection_color.Sample(sampler_tex, input.texcoord + float2(i * separation, j * separation)).rgb;
            count += 1.0;
        }
    }

    color.rgb /= count;
    output.color = color;
    return output;
}
