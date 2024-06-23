#include "interface.hlsl"

V2P VSMain(VSIn input) {
    V2P output;

    const float4 pos = float4(input.position, 1);
    output.position = mul(modelProperties.matrixMVP, pos);
    output.position_world = mul(modelProperties.matrixM, pos).xyz;

    output.normal_world = ModelToWorldNormal(-input.normal);

    output.texcoord = input.texcoord;

    output.depth = output.position;

    return output;
}