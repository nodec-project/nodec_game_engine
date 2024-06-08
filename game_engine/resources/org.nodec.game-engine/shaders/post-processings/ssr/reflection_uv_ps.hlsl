#include "reflection_uv_interface.hlsl"

struct PSOut {
    float4 reflection_uv: SV_TARGET0;
};

PSOut PSMain(V2P input) : SV_TARGET {
    PSOut output;

    float max_distance = 8;
    float resolution = 0.3;
    int steps = 5;
    float thickness = 0.5;

    float screen_tex_width, screen_tex_height;
    texScreen.GetDimensions(screen_tex_width, screen_tex_height);

    float4 out_uv = float4(0, 0, 0, 0);

    float non_linear_depth = texDepth.Sample(sampler_tex, input.texcoord).r;
    if (non_linear_depth <= 0) {
        output.reflection_uv = out_uv;
        return output;
    }
    const float3 position_from = ViewSpacePosition(non_linear_depth, input.texcoord, sceneProperties.matrixPInverse);

    const float3 normal_in_world_space = texNormal.Sample(sampler_tex, input.texcoord);
    const float3 normal = normalize(mul(sceneProperties.matrixV, normal_in_world_space));
    const float3 unit_position_from = normalize(position_from);
    const float3 pivot = normalize(reflect(unit_position_from, normal));

    float3 position_to = position_from;
    
    float4 start_view = float4(position_from.xyz + (pivot * 0.0), 1.0);
    float4 end_view = float4(position_from.xyz + (pivot * max_distance), 1.0);

    float4 start_frag = start_view;
    start_frag = mul(sceneProperties.matrixP, start_frag);
    start_frag.xyz /= start_frag.w;
    start_frag.xy = start_frag.xy * 0.5 + 0.5;
    start_frag.xy *= float2(screen_tex_width, screen_tex_height);

    float4 end_frag = end_view;
    end_frag = mul(sceneProperties.matrixP, end_frag);
    end_frag.xyz /= end_frag.w;
    end_frag.xy = end_frag.xy * 0.5 + 0.5;
    end_frag.xy *= float2(screen_tex_width, screen_tex_height);

    float2 frag = start_frag.xy;
    out_uv.xy = frag / float2(screen_tex_width, screen_tex_height);

    float delta_x = end_frag.x - start_frag.x;
    float delta_y = end_frag.y - start_frag.y;
    float use_x = abs(delta_x) >= abs(delta_y) ? 1 : 0;
    float delta = lerp(abs(delta_y), abs(delta_x), use_x) * clamp(resolution, 0, 1);
    float2 increment = float2(delta_x, delta_y) / max(delta, 0.001);

    float search0 = 0;
    float search1 = 0;

    int hit0 = 0;
    int hit1 = 0;

    float view_distance = start_view.z;
    float depth = thickness;

    float i = 0;

    [unroll(64)]
    for (i = 0; i < int(delta); ++i) {
        frag += increment;
        out_uv.xy = frag / float2(screen_tex_width, screen_tex_height);
        non_linear_depth = texDepth.Sample(sampler_tex, out_uv).r;
        position_to = ViewSpacePosition(non_linear_depth, out_uv, sceneProperties.matrixPInverse);

        search1 = lerp((frag.y - start_frag.y) / delta_y, (frag.x - start_frag.x) / delta_x, use_x);
        search1 = clamp(search1, 0, 1);
        view_distance = (start_view.z * end_view.z) / lerp(end_view.z, start_view.z, search1);
        depth = view_distance - position_to.z;

        if (depth > 0 && depth < thickness) {
            hit0 = 1;
            break;
        } else {
            search0 = search1;
        }

    }

    search1 = search0 + ((search1 - search0) * 0.5);
    steps *= hit0;

    for (i = 0; i < steps; ++i) {
        frag = lerp(start_frag, end_frag, search1);
        out_uv.xy = frag / float2(screen_tex_width, screen_tex_height);
        non_linear_depth = texDepth.Sample(sampler_tex, out_uv).r;
        position_to = ViewSpacePosition(non_linear_depth, out_uv, sceneProperties.matrixPInverse);

        view_distance = (start_view.z * end_view.z) / lerp(end_view.z, start_view.z, search1);
        depth = view_distance - position_to.z;

        if (depth > 0 && depth < thickness) {
            hit1 = 1;
            search1 = search0 + ((search1 - search0) * 0.5);
        } else {
            float temp = search1;
            search1 = search1 + ((search1 - search0) * 0.5);
            search0 = temp;
        }
    }

    float visibility = hit1 * non_linear_depth
        * (1 - max(dot(-unit_position_from, pivot), 0))
        * (1 - clamp(depth / thickness, 0, 1))
        * (1 - clamp(length(position_to - position_from) / max_distance, 0, 1))
        * (out_uv.x < 0 || out_uv.x > 1 ? 0 : 1)
        * (out_uv.y < 0 || out_uv.y > 1 ? 0 : 1);
    visibility = clamp(visibility, 0, 1);
    out_uv.ba = visibility;
    // out_uv.ba = float2(visibility, visibility);
    // out_uv.a = 1;

    output.reflection_uv = out_uv;
    return output;
}