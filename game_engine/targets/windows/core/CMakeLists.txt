cmake_minimum_required(VERSION 3.10)

project(nodec_game_engine_core LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
    src/Engine.cpp
    src/imgui_impl_dx11.cpp
    src/imgui_impl_win32.cpp
    src/Logging.cpp
    src/Window.cpp
    src/Graphics/DxgiInfoLogger.cpp
    src/Graphics/Graphics.cpp
    src/Physics/PhysicsSystemBackend.cpp
    src/Rendering/SceneRenderer.cpp
    src/SceneAudio/SceneAudioSystem.cpp
    src/SceneSerialization/SceneSerializationBackend.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC include
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC 
    UNICODE _UNICODE
    CEREAL_THREAD_SAFE=1
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    nodec
    nodec_application
    nodec_input
    nodec_rendering
    nodec_resources
    nodec_scene
    nodec_scene_audio
    nodec_serialization
    nodec_scene_serialization
    nodec_screen
    nodec_world
    nodec_animations
    nodec_physics
    imgui
    freetype
    DirectXTex
    LinearMath
    BulletDynamics
    BulletCollision
    nodec_bullet3_compat
)
