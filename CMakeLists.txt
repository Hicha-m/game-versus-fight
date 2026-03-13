cmake_minimum_required(VERSION 3.16)

project(game_versus_fight C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image CONFIG QUIET)

set(GAME_SDL3_IMAGE_ENABLED 0)
set(GAME_SDL3_IMAGE_HEADER 0)
set(GAME_SDL3_IMAGE_RUNTIME_LIB "")

if(SDL3_image_FOUND)
    set(GAME_SDL3_IMAGE_ENABLED 1)
    set(GAME_SDL3_IMAGE_HEADER 1)
else()
    find_path(SDL3_IMAGE_INCLUDE_DIR
        NAMES SDL3_image/SDL_image.h
    )
    if(SDL3_IMAGE_INCLUDE_DIR)
        set(GAME_SDL3_IMAGE_HEADER 1)
    endif()

    find_library(SDL3_IMAGE_RUNTIME_LIB
        NAMES SDL3_image SDL3_image.so.0 libSDL3_image.so.0
        PATHS /usr/lib64 /lib64 /usr/lib /lib
    )

    if(SDL3_IMAGE_RUNTIME_LIB)
        set(GAME_SDL3_IMAGE_ENABLED 1)
    endif()
endif()

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
    src/engine/animation.c
    src/engine/assets.c
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

if(SDL3_image_FOUND)
    target_link_libraries(game_core PUBLIC SDL3_image::SDL3_image)
elseif(GAME_SDL3_IMAGE_RUNTIME_LIB)
    target_link_libraries(game_core PUBLIC ${GAME_SDL3_IMAGE_RUNTIME_LIB})
endif()

if(GAME_SDL3_IMAGE_ENABLED)
    target_compile_definitions(game_core PUBLIC
        GAME_HAS_SDL3_IMAGE=1
        GAME_HAS_SDL3_IMAGE_HEADER=${GAME_SDL3_IMAGE_HEADER}
    )
else()
    message(WARNING "SDL3_image not found: PNG textures disabled, using primitive fallback rendering")
    target_compile_definitions(game_core PUBLIC GAME_HAS_SDL3_IMAGE=0)
endif()

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
