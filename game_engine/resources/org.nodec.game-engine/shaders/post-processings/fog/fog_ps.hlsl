#include "fog_interface.hlsl"

// Implement fog effect.
//
// Reference:
// * https://lettier.github.io/3d-game-shaders-for-beginners/fog.html


float4 PSMain(V2P input) : SV_TARGET {
    float4 color = texScreen.Sample(sampler_tex, input.texcoord);
    const float non_linear_depth = texDepth.Sample(sampler_tex, input.texcoord).r;
    const float3 position = ViewSpacePosition(non_linear_depth, input.texcoord, sceneProperties.matrixPInverse);
    
    float fog_min = 0.00;
    float fog_max = 0.97;

    const float near = materialProperties.near_far.x;
    const float far = materialProperties.near_far.y;

    const float intensity = clamp((position.z - near) / (far - near), fog_min, fog_max);
    const float4 fog_color = float4(materialProperties.color.rgb, intensity);

    color = lerp(color, fog_color, intensity);
    return color;
}