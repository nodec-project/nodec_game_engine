#include "interface.hlsl"

#define PI 3.14159265359f

// https://github.com/TwoTailsGames/Unity-Built-in-Shaders/blob/master/DefaultResourcesExtra/Skybox-Panoramic.shader

float2 ToRadialCoords(float3 coords) {
    // normalized cube -> sphere
    float3 normalizedCoords = normalize(coords); 

    // y: -1 -> +1 
    // =>
    // theta: pi <- 0
    float latitude = acos(normalizedCoords.y);
    
    float longitude = atan2(normalizedCoords.z, normalizedCoords.x);

    return float2(0.5 - longitude * 0.5 / PI, latitude / PI);
}

float4 PSMain(V2P input) : SV_TARGET {
    float4 color = float4(input.texcoord, 1);
    float2 tc = ToRadialCoords(input.texcoord);

    if (textureConfig.texHasFlag & 0x01) {
        color = texMain.Sample(sampler_texMain, tc);
    }

    //return float4(input.texcoord.y, 0, 0, 1);
    return color;
}