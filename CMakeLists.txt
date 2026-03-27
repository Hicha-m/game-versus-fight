cmake_minimum_required(VERSION 3.16)

project(game_versus_fight C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 REQUIRED CONFIG)

add_library(game_core STATIC
    src/engine/engine.c
    src/engine/input.c

    src/game/arena/arena.c
    src/game/combat/combat.c
    src/game/ai/ai.c
    src/game/game.c

    src/render/render.c

    src/utils/log.c
    src/utils/utils.c
    src/utils/memory.c
)

target_include_directories(game_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(game_core PRIVATE /W4)
else()
    target_compile_options(game_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

add_executable(game_app
    src/main.c
)

target_link_libraries(game_app PRIVATE game_core)
target_link_libraries(game_core PUBLIC SDL3::SDL3)  

if(NOT MSVC AND NOT WIN32)  
    target_link_libraries(game_core PUBLIC m)  
endif()  

option(GAME_BUILD_TESTS "Build legacy tests" OFF)

if (GAME_BUILD_TESTS)
    enable_testing()

    target_compile_definitions(game_core PRIVATE GAME_TEST_MODE=1)

    add_executable(game_tests
        tests/test_main.c
        tests/test_ai.c
        tests/test_arena.c
        tests/test_combat.c
        tests/test_engine.c
        tests/test_game.c
    )

    target_link_libraries(game_tests PRIVATE game_core)

    add_test(NAME game_tests COMMAND game_tests)
    add_test(NAME architecture_rules COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_architecture.sh)
endif()

add_custom_target(run_game
    COMMAND $<TARGET_FILE:game_app>
    DEPENDS game_app
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Build and run game_app"
)
