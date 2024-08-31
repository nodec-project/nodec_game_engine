project(nodec_game_engine_main)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE
    src/win_main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    nodec_game_engine_core
)
