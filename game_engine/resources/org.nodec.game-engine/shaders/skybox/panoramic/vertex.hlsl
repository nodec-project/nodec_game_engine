#include "interface.hlsl"

V2P VSMain(VSIn input) {
    V2P output;
    const float4 pos = float4(input.position, 1);
    const float4x4 matrixV = float4x4(
        float4(sceneProperties.matrixV[0].xyz, 0),
        float4(sceneProperties.matrixV[1].xyz, 0),
        float4(sceneProperties.matrixV[2].xyz, 0),
        float4(0, 0, 0, 1));
    output.position = mul(matrixV, pos);
    output.position = mul(sceneProperties.matrixP, output.position);

    output.texcoord = pos.xyz;
    return output;
}