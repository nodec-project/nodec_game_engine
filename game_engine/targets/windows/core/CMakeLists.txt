cmake_minimum_required(VERSION 3.10)

project(nodec_game_engine_core LANGUAGES CXX)

find_package(unofficial-uwebsockets CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC
    src/engine.cpp

    # src/graphics/DxgiInfoLogger.cpp
    src/graphics/graphics.cpp
    src/graphics/graphics_device.cpp
    src/imgui_impl_dx11.cpp
    src/imgui_impl_win32.cpp
    src/input/mouse_device_backend.cpp
    src/logging.cpp
    src/physics/physics_system_backend.cpp
    src/rendering/scene_renderer_context.cpp
    src/rendering/scene_renderer.cpp
    src/rendering/scene_rendering_context.cpp
    src/rendering/occlusion_culling_system.cpp  # オクルージョンカリングシステムを追加
    src/resources/resource_loader.cpp
    src/scene_audio/scene_audio_system.cpp
    src/scene_serialization/scene_serialization_backend.cpp
    src/screen/screen_backend.cpp
    src/window.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN UWS_NO_ZLIB)

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
    nodec_animation
    nodec_physics
    nodec_ui
    imgui
    freetype
    DirectXTex
    LinearMath
    BulletDynamics
    BulletCollision
    nodec_bullet3_compat
    unofficial::uwebsockets::uwebsockets
)
