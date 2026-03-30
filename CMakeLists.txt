cmake_minimum_required(VERSION 3.16)

project(game_versus_fight C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 REQUIRED CONFIG)

# Try to find SDL3_image, but don't fail if not found (will use BMP fallback)
find_package(SDL3-image CONFIG)

add_library(game_core STATIC
    src/engine/engine.c
    src/engine/input.c

    src/game/arena/arena.c
    src/game/arena/arena_gen.c
    src/game/arena/arena_validate.c
    src/game/combat/combat.c
    src/game/combat/character.c
    src/game/combat/physics.c
    src/game/combat/attacks.c
    src/game/ai/ai.c
    src/game/game.c
    src/game/config.c

    src/ui/menu.c

    src/render/render.c
    src/render/render_menu.c
    src/render/render_victory.c
    src/render/animation.c
    src/render/assets.c

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

# Link SDL3_image if found, and set compile definition
if(SDL3-image_FOUND)
    target_link_libraries(game_core PUBLIC SDL3-image::SDL3-image)
    target_compile_definitions(game_core PUBLIC GAME_HAS_SDL3_IMAGE=1)
    
    # Try to find SDL3_image headers
    find_path(SDL3_IMAGE_INCLUDE_DIR NAMES SDL3_image/SDL_image.h SDL_image.h)
    if(SDL3_IMAGE_INCLUDE_DIR)
        target_compile_definitions(game_core PUBLIC GAME_HAS_SDL3_IMAGE_HEADER=1)
    endif()
    
    message(STATUS "SDL3_image found - PNG support enabled")
else()
    message(STATUS "SDL3_image not found - using BMP fallback only")
endif()

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
