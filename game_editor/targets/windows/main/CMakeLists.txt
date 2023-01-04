cmake_minimum_required(VERSION 3.10)

project(nodec_game_editor)

add_library(${PROJECT_NAME} STATIC
    src/Editor.cpp
    src/ResourceExporter.cpp
    src/WinMain.cpp
    src/EditorWindows/SceneViewWindow.cpp
    src/InspectorGUI/InspectorGUI.cpp
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    imgui
    imwindows
    imessentials
    nodec_scene_editor
    imguizmo

    PRIVATE
    nodec_game_engine_core
    assimp
)