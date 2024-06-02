if(WIN32)
    add_subdirectory(targets/windows/core)
else()
    message(FATAL_MESSAGE "This platform does not supported.")
endif()

# https://stackoverflow.com/questions/71299716/how-to-compile-hlsl-shaders-during-build-with-cmake
add_custom_target(nodec_game_engine_shaders)

if(MSVC)
    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/image/vertex.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/image/pixel.hlsl")

    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/pbr/vertex.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/pbr/pixel.hlsl")

    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/pbr-defer/geometry_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/pbr-defer/geometry_ps.hlsl")
    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/pbr-defer/lighting_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/pbr-defer/lighting_ps.hlsl")

    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/bloom/blend_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/bloom/blend_ps.hlsl")
    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/bloom/blur_h_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/bloom/blur_h_ps.hlsl")
    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/bloom/blur_v_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/bloom/blur_v_ps.hlsl")
    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/bloom/brightness_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/bloom/brightness_ps.hlsl")

    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/hdr/hdr_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/hdr/hdr_ps.hlsl")

    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/fog/fog_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/fog/fog_ps.hlsl")

    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/ssao/occlusion_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/ssao/occlusion_ps.hlsl")

    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/ssr/reflection_uv_vs.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/post-processings/ssr/reflection_uv_ps.hlsl")

    nodec_add_vertex_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/skybox/panoramic/vertex.hlsl")
    nodec_add_pixel_shader(nodec_game_engine_shaders
        "resources/org.nodec.game-engine/shaders/skybox/panoramic/pixel.hlsl")
endif()

option(NODEC_GAME_ENGINE_BUILD_SHADERS OFF)

if(NODEC_GAME_ENGINE_BUILD_SHADERS)
    add_dependencies(nodec_game_engine nodec_game_engine_shaders)
endif()