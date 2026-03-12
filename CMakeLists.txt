cmake_minimum_required(VERSION 3.16)

project(game_versus_fight C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 REQUIRED CONFIG)

add_library(game_core STATIC
    src/ai/ai.c
    src/ai/cache.c
    src/ai/difficulty.c
    src/ai/evaluator.c
    src/ai/minmax.c
    src/ai/state.c

    src/arena/arena.c
    src/arena/generator.c
    src/arena/seed.c
    src/arena/tiles.c
    src/arena/validator.c

    src/combat/attacks.c
    src/combat/character.c
    src/combat/combat.c
    src/combat/momentum.c
    src/combat/physics.c
    src/combat/sword.c

    src/engine/audio.c
    src/engine/debug.c
    src/engine/engine.c
    src/engine/input.c
    src/engine/render.c
    src/engine/timing.c

    src/game/config.c
    src/game/game.c
    src/game/hud.c
    src/game/match.c
    src/game/menu.c

    src/utils/logging.c
    src/utils/math.c
    src/utils/memory.c
    src/utils/utils.c
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
target_link_libraries(game_core PUBLIC SDL3::SDL3 m)

enable_testing()

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

add_custom_target(run_game
    COMMAND $<TARGET_FILE:game_app>
    DEPENDS game_app
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Build and run game_app"
)
