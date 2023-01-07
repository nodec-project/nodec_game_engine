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

add_subdirectory(nodec/nodec)

# set(THREAD_SAFE ON)
add_subdirectory(nodec/modules/nodec_application)
add_subdirectory(nodec/modules/nodec_input)
add_subdirectory(nodec/modules/nodec_rendering)
add_subdirectory(nodec/modules/nodec_resources)
add_subdirectory(nodec/modules/nodec_scene)
add_subdirectory(nodec/modules/nodec_scene_audio)
add_subdirectory(nodec/modules/nodec_scene_serialization)
add_subdirectory(nodec/modules/nodec_screen)
add_subdirectory(nodec/modules/nodec_serialization)
add_subdirectory(nodec/modules/nodec_world)
add_subdirectory(nodec/modules/nodec_animations)
add_subdirectory(nodec/modules/nodec_physics)

if(NODEC_GAME_EDITOR_ENABLED)
    set(BUILD_SHARED_LIBS ON)
    add_subdirectory(nodec/modules/assimp/assimp)

    add_subdirectory(nodec/modules/imgui)
    add_subdirectory(nodec/modules/imessentials)
    add_subdirectory(nodec/modules/imwindows)
    add_subdirectory(nodec/modules/imguizmo)
    add_subdirectory(nodec/modules/nodec_scene_editor)
endif()

add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
    nodec
    nodec_application
    nodec_animations
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

add_subdirectory(nodec/platforms/sdks/bullet3)

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
add_subdirectory(nodec/platforms/sdks/nodec_bullet3_compat)

add_subdirectory(nodec/platforms/sdks/DirectXTex/DirectXTex)
add_subdirectory(nodec/platforms/sdks/freetype)

if(WIN32)
    add_subdirectory(game_engine/targets/windows/core)

    if(NODEC_GAME_EDITOR_ENABLED)
        # if(NOT WIN32)
        # message(FATAL_ERROR "Editor mode is still only supported on Windows.")
        # endif()
        add_subdirectory(game_editor/targets/windows/main)

        target_link_libraries(${PROJECT_NAME}
            INTERFACE
            nodec_game_editor
        )
    else()
    endif()
else()
    message(FATAL_MESSAGE "This platform does not supported.")
endif()

# === Configure shader build system ===
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    SHADER_COMMON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/game_engine/resources/org.nodec.game-engine/shaders
)

# https://stackoverflow.com/questions/71299716/how-to-compile-hlsl-shaders-during-build-with-cmake
add_custom_target(nodec_game_engine_shaders)

if(MSVC)
    function(add_vertex_shader LIST FILE)
        set(${LIST} ${${LIST}} ${FILE} PARENT_SCOPE)
        set_source_files_properties(${FILE}
            PROPERTIES
            SHADER_TYPE "vs"
            SHADER_MODEL "4_0"
            ENTRY_POINT_NAME "VSMain"
        )
    endfunction()

    function(add_pixel_shader LIST FILE)
        set(${LIST} ${${LIST}} ${FILE} PARENT_SCOPE)
        set_source_files_properties(${FILE}
            PROPERTIES
            SHADER_TYPE "ps"
            SHADER_MODEL "4_0"
            ENTRY_POINT_NAME "PSMain"
        )
    endfunction()

    set(HLSL_SHADER_FILES "")

    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/image/vertex.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/pbr/vertex.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/pbr-defer/geometry_vs.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/pbr-defer/lighting_vs.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/bloom/blend_vs.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/bloom/blur_h_vs.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/bloom/blur_v_vs.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/bloom/brightness_vs.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/hdr/hdr_vs.hlsl")
    add_vertex_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/ssao/occlusion_vs.hlsl")

    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/image/pixel.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/pbr/pixel.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/pbr-defer/geometry_ps.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/pbr-defer/lighting_ps.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/bloom/blend_ps.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/bloom/blur_h_ps.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/bloom/blur_v_ps.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/bloom/brightness_ps.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/hdr/hdr_ps.hlsl")
    add_pixel_shader(HLSL_SHADER_FILES
        "game_engine/resources/org.nodec.game-engine/shaders/post-processings/ssao/occlusion_ps.hlsl")

    foreach(FILE ${HLSL_SHADER_FILES})
        get_target_property(SHADER_COMMON_INCLUDE_DIR ${PROJECT_NAME} SHADER_COMMON_INCLUDE_DIR)
        get_filename_component(FILE_WE ${FILE} NAME_WE)
        get_filename_component(DIRECTORY ${FILE} DIRECTORY)
        get_source_file_property(SHADER_TYPE ${FILE} SHADER_TYPE)
        get_source_file_property(SHADER_MODEL ${FILE} SHADER_MODEL)
        get_source_file_property(ENTRY_POINT_NAME ${FILE} ENTRY_POINT_NAME)

        add_custom_command(TARGET nodec_game_engine_shaders
            #OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${FILE_WE}.cso
            COMMAND fxc.exe /nologo /E${ENTRY_POINT_NAME} /T${SHADER_TYPE}_${SHADER_MODEL} /I ${SHADER_COMMON_INCLUDE_DIR} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fo ${DIRECTORY}/${FILE_WE}.cso /Fd ${DIRECTORY}/${FILE_WE}.pdb ${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM)

        # add_dependencies(nodec_game_engine_shaders ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${FILE_WE}.cso)
    endforeach(FILE)
endif()

add_dependencies(${PROJECT_NAME} nodec_game_engine_shaders)

# END Configure shader build system ===
