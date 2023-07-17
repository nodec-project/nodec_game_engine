cmake_minimum_required(VERSION 3.10)

project(nodec_game_editor)

add_library(${PROJECT_NAME} STATIC
    src/Editor.cpp
    src/InspectorGUI/InspectorGUI.cpp
    src/WinMain.cpp
    src/editor_windows/scene_hierarchy_window.cpp
    src/editor_windows/scene_view_window.cpp
    src/editor_windows/material_editor_window.cpp
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    imgui
    imessentials
    nodec_scene_editor
    imguizmo

    PRIVATE
    nodec_game_engine_core
    assimp
)