cmake_minimum_required(VERSION 3.10)

project(nodec_game_engine)

if(WIN32)
    set(CMAKE_OBJECT_PATH_MAX 1280)
endif()

if(MSVC)
    add_compile_options(/bigobj)
endif()

set(CMAKE_CXX_STANDARD 14)

option(NODEC_GAME_EDITOR_ENABLED ON)

add_subdirectory(nodec)

# set(THREAD_SAFE ON)
add_subdirectory(modules/nodec_application)
add_subdirectory(modules/nodec_input)

set(NODEC_RENDERING_ENABLE_SERIALIZATION ON CACHE BOOL "" FORCE)
add_subdirectory(modules/nodec_rendering)

add_subdirectory(modules/nodec_resources)

set(NODEC_SCENE_ENABLE_SERIALIZATION ON CACHE BOOL "" FORCE)
add_subdirectory(modules/nodec_scene)

set(NODEC_SCENE_AUDIO_ENABLE_SERIALIZATION ON CACHE BOOL "" FORCE)
add_subdirectory(modules/nodec_scene_audio)

add_subdirectory(modules/nodec_serialization)
add_subdirectory(modules/nodec_scene_serialization)
add_subdirectory(modules/nodec_screen)
add_subdirectory(modules/nodec_world)
add_subdirectory(modules/nodec_animation)

set(NODEC_PHYSICS_ENABLE_SERIALIZATION ON CACHE BOOL "" FORCE)
add_subdirectory(modules/nodec_physics)

if(NODEC_GAME_EDITOR_ENABLED)
    set(BUILD_SHARED_LIBS ON)
    set(ASSIMP_BUILD_ZLIB ON)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
    set(ASSIMP_BUILD_ASSIMP_VIEW OFF)
    add_subdirectory(modules/assimp)

    add_subdirectory(modules/imgui)
    add_subdirectory(modules/imessentials)
    add_subdirectory(modules/imguizmo)
    add_subdirectory(modules/nodec_scene_editor)
endif()

add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
    nodec
    nodec_application
    nodec_animation
    nodec_input
    nodec_rendering
    nodec_resources
    nodec_scene
    nodec_scene_audio
    nodec_scene_serialization
    nodec_screen
    nodec_serialization
    nodec_world
    nodec_physics
)

target_compile_definitions(${PROJECT_NAME}
    INTERFACE
    CEREAL_THREAD_SAFE=1
)

# == bullet3 configuration ===
# https://github.com/bulletphysics/bullet3/pull/3860
set(BUILD_SHARED_LIBS OFF)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)

add_subdirectory(sdks/bullet3)

target_include_directories(Bullet3Collision PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(Bullet3Common PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(Bullet3Dynamics PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(Bullet3Geometry PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(Bullet3OpenCL_clew PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(Bullet2FileLoader PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(BulletCollision PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(BulletDynamics PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(BulletInverseDynamics PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(BulletSoftBody PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)
target_include_directories(LinearMath PUBLIC ${BULLET_PHYSICS_SOURCE_DIR}/src)

# End bullet3 configuration ===
add_subdirectory(sdks/nodec_bullet3_compat)

add_subdirectory(sdks/DirectXTex)
add_subdirectory(sdks/freetype)

# === Configure shader build system ===
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    SHADER_COMMON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/game_engine/resources/org.nodec.game-engine/shaders
)

if(MSVC)
    function(nodec_add_vertex_shader TARGET FILE)
        get_target_property(SHADER_COMMON_INCLUDE_DIR ${PROJECT_NAME} SHADER_COMMON_INCLUDE_DIR)
        get_filename_component(FILE_WE ${FILE} NAME_WE)
        get_filename_component(DIRECTORY ${FILE} DIRECTORY)
        add_custom_command(TARGET ${TARGET}
            COMMAND fxc.exe /nologo /EVSMain /Tvs_4_0 /I ${SHADER_COMMON_INCLUDE_DIR} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fo ${DIRECTORY}/${FILE_WE}.cso /Fd ${DIRECTORY}/${FILE_WE}.pdb ${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    endfunction()

    function(nodec_add_pixel_shader TARGET FILE)
        get_target_property(SHADER_COMMON_INCLUDE_DIR ${PROJECT_NAME} SHADER_COMMON_INCLUDE_DIR)
        get_filename_component(FILE_WE ${FILE} NAME_WE)
        get_filename_component(DIRECTORY ${FILE} DIRECTORY)
        add_custom_command(TARGET ${TARGET}
            COMMAND fxc.exe /nologo /EPSMain /Tps_4_0 /I ${SHADER_COMMON_INCLUDE_DIR} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fo ${DIRECTORY}/${FILE_WE}.cso /Fd ${DIRECTORY}/${FILE_WE}.pdb ${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    endfunction()
endif()

# END Configure shader build system ===
add_subdirectory(game_engine)

if(NODEC_GAME_EDITOR_ENABLED)
    add_subdirectory(game_editor)

    target_link_libraries(${PROJECT_NAME}
        INTERFACE
        nodec_game_editor_main
    )
else()
    # TODO: link engine main library
endif()
