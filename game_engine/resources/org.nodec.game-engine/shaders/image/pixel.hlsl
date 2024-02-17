#include "interface.hlsl"

float4 PSMain(V2P input) : SV_Target {
    float4 color = materialProperties.color;

    if (textureConfig.texHasFlag & 0x01) {
        float4 pixel = texImage.Sample(sampler_texImage, input.texcoord);
        color *= pixel;
    }

    if (textureConfig.texHasFlag & 0x02) {
        float mask = texMask.Sample(sampler_texMask, input.texcoord);
        color.a *= mask;
    }

    if (color.a < 0.1f) {
        discard;
    }

    return color;
}