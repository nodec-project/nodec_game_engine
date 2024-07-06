#include "interface.hlsl"

V2P VSMain(VSIn input) {
    V2P output;

    const float4 pos = float4(input.position, 1);
    output.position = mul(modelProperties.matrixMVP, pos);
    output.position_world = mul(modelProperties.matrixM, pos).xyz;


    float3 normal_world = ModelToWorldNormal(input.normal);
    float3 forward_camera = normalize(sceneProperties.cameraPos.xyz - output.position_world);

    if (dot(normal_world, forward_camera) < 0) {
        normal_world = -normal_world;
    }
    output.normal_world = normal_world;

    output.texcoord = input.texcoord;

    output.depth = output.position;

    return output;
}