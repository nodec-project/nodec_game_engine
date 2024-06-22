#include "interface.hlsl"


 struct PSOut {
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 depth : SV_TARGET3;
 };

PSOut PSMain(V2P input) : SV_Target {
    PSOut output;

    float4 color = materialProperties.color;

    if (textureConfig.texHasFlag & 0x01) {
        float4 pixel = texImage.Sample(sampler_texImage, input.texcoord);
        color *= pixel;
    }

    if (textureConfig.texHasFlag & 0x02) {
        float mask = texMask.Sample(sampler_texMask, input.texcoord);
        color.a *= mask;
    }

    if (color.a < 0.01f) {
        discard;
    }

    output.color = color;
    
    float3 normal_world = normalize(input.normal_world);
    output.normal = float4(normal_world, 1);

    float depth = input.depth.z / input.depth.w;
    output.depth = float4(depth.xxxx);

    return output;
}