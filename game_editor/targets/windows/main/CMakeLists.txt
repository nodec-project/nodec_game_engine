project(nodec_game_editor_main)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE
    src/editor.cpp
    src/WinMain.cpp
    src/component_editors/physics_shape_editor.cpp
    src/component_editors/prefab_editor.cpp
    src/editor_windows/entity_inspector_window.cpp
    src/editor_windows/material_editor_window.cpp
    src/editor_windows/scene_hierarchy_window.cpp
    src/editor_windows/scene_view_window.cpp
    src/editor_windows/geometry_buffer_inspector_window.cpp
    src/scene_gizmo_renderer.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    nodec_game_engine_core
    assimp

    PUBLIC
    imgui
    imessentials
    nodec_scene_editor
    imguizmo
)
