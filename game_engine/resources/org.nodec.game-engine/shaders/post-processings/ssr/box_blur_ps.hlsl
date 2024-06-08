#include "box_blur_interface.hlsl"

struct PSOut {
    float4 color : SV_Target0;
};

PSOut PSMain(V2P input) {
    PSOut output;

    float tex_width, tex_height;
    tex_reflection_color.GetDimensions(tex_width, tex_height);

    float4 color = tex_reflection_color.Sample(sampler_tex, input.texcoord);
    // color.a = 1; // ???
    
    // return color;
    
    int size = 1;
    if (size <= 0) {
        // return color;
        output.color = color;
        return output;
    }

    int separation = 1;
    separation = max(separation, 1);

    color = float4(0, 0, 0, 0);
    float count = 0.0;

    for (int i = -size; i <= size; ++i) {
        for (int j = -size; j <= size; ++j) {
            color += tex_reflection_color.Sample(sampler_tex,
                input.texcoord + float2(i * separation / tex_width, j * separation / tex_height));
            count += 1.0;
        }
    }

    color /= count;
    // color.a = 1;
    
    // return color;
    output.color = color;
    return output;
}
