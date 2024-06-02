#include "reflection_uv_interface.hlsl"

float4 PSMain(V2P input) : SV_TARGET {
    float max_distance = 15;
    float resolution = 0.5;
    int steps = 10;
    float thickness = 0.5;

    const float non_linear_depth = texDepth.Sample(sampler_tex, input.texcoord).r;
    const float3 position = ViewSpacePosition(non_linear_depth, input.texcoord, sceneProperties.matrixPInverse);
    const float3 normal_in_world_space = texNormal.Sample(sampler_tex, input.texcoord);
    const float3 normal = normalize(mul(sceneProperties.matrixV, normal_in_world_space));

    return float4(normal.xyz, 1);
}