#include "interface.hlsl"

float4 PSMain(V2P input) : SV_Target {
    return materialProperties.color;
}