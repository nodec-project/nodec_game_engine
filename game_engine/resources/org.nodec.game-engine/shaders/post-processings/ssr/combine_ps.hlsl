#include "combine_interface.hlsl"

#include "../../common/brdf.hlsl"

float4 PSMain(V2P input) : SV_TARGET {
    float4 base_color = tex_screen.Sample(sampler_tex, input.texcoord);
    float4 reflection_color = tex_reflection_color.Sample(sampler_tex, input.texcoord);
    float4 normal = tex_normal.Sample(sampler_tex, input.texcoord);
    float4 mat_props = tex_mat_props.Sample(sampler_tex, input.texcoord);
    float non_linear_depth = tex_depth.Sample(sampler_tex, input.texcoord).r;
    float3 position_in_view = ViewSpacePosition(non_linear_depth, input.texcoord, sceneProperties.matrixPInverse);

    const float3 view_dir = normalize(position_in_view);

    BRDFSurface surface;
    surface.albedo = base_color.rgb;
    surface.normal = normal.rgb;
    surface.roughness = mat_props.r;
    surface.metallic = mat_props.g;

    float3 ambient = EnvironmentBRDF(surface, -view_dir, float3(0, 0, 0), reflection_color);

    // return float4(ambient, 1);
    float3 color = base_color.rgb + ambient;
    return float4(color, 1);


    // const float3 normal_in_world_space = tex_normal.Sample(sampler_tex, input.texcoord);
    // return float4(normal_in_world_space, 1);
    

    // return lerp(float4(0,0 ,0, 0), reflection_color, reflection_color.a);
    // return reflection_color;

    // return float4(base_color.rgb, 1);
    // return base_color;
    // return reflection_color;
    // return float4(reflection_color.r, 0, 0, 1); 

    // float4 color = base_color;
    // color.rgb = lerp(base_color.rgb, reflection_color.rgb, clamp(reflection_color.a, 0, 1));
    // return float4(color.rgb, 1);
}
