#include "common/interface_model.hlsl"

struct V2P {
    float4 position : SV_Position;
};

struct MaterialProperties {
    float4 color;
};

cbuffer cbMaterialProperties : register(b3)
{
    MaterialProperties materialProperties;
};
