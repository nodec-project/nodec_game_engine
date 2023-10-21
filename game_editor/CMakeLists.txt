if(WIN32)
    add_subdirectory(targets/windows/main)
else()
    message(FATAL_MESSAGE "This platform does not supported.")
endif()

add_custom_target(nodec_game_editor_shaders)
if(MSVC)
    nodec_add_vertex_shader(nodec_game_editor_shaders
        "resources/org.nodec.game-editor/shaders/gizmo-wire/vertex.hlsl"
    )
    nodec_add_pixel_shader(nodec_game_editor_shaders
        "resources/org.nodec.game-editor/shaders/gizmo-wire/pixel.hlsl"
    )
endif()

option(NODEC_GAME_EDITOR_BUILD_SHADERS OFF)

if(NODEC_GAME_EDITOR_BUILD_SHADERS)
    add_dependencies(nodec_game_editor_main nodec_game_editor_shaders)
endif()