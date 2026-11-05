cmake_minimum_required(VERSION 3.25)

project(game_versus_fight C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 REQUIRED CONFIG)

# Try to find SDL3_image, but don't fail if not found (will use BMP fallback)
find_package(SDL3_image CONFIG)

add_library(game_core STATIC
    src/engine/audio.c
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
    src/game/menu.c

    src/render/render.c
    src/render/render_menu.c
    src/render/render_victory.c
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
if(SDL3_image_FOUND)
    target_link_libraries(game_core PUBLIC SDL3_image::SDL3_image)
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

option(GAME_BUILD_TESTS "Build regression and integration tests" ON)

if (GAME_BUILD_TESTS)
    enable_testing()

    # Production and tests use the same simulation and platform build.

    add_executable(game_tests
        tests/test_main.c
        tests/test_ai.c
        tests/test_arena.c
        tests/test_combat.c
        tests/test_engine.c
        tests/test_game.c
    )

    target_link_libraries(game_tests PRIVATE game_core)

    add_executable(nidhogg_tests tests/test_nidhogg.c)
    target_link_libraries(nidhogg_tests PRIVATE game_core)
    add_test(NAME nidhogg_tests COMMAND nidhogg_tests)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/comparison-work)
    set_tests_properties(nidhogg_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/comparison-work)

    add_executable(audit_tests tests/test_audit.c)
    target_link_libraries(audit_tests PRIVATE game_core)
    add_test(NAME audit_tests COMMAND audit_tests)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/audit-work)
    set_tests_properties(audit_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/audit-work)

    add_executable(regression_tests tests/test_regressions.c)
    target_link_libraries(regression_tests PRIVATE game_core)
    add_test(NAME regression_tests COMMAND regression_tests)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/regression-work)
    set_tests_properties(regression_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/regression-work)
    add_executable(generation_tests tests/test_generation.c)
    target_link_libraries(generation_tests PRIVATE game_core)
    add_test(NAME generation_tests COMMAND generation_tests)
    add_test(NAME game_tests COMMAND game_tests)
    add_test(NAME architecture_rules COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_architecture.sh)
    set_tests_properties(nidhogg_tests audit_tests game_tests regression_tests generation_tests PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software;SDL_AUDIODRIVER=dummy" TIMEOUT 120)
    if(MSVC)
        target_compile_options(nidhogg_tests PRIVATE /UNDEBUG)
        target_compile_options(audit_tests PRIVATE /UNDEBUG)
        target_compile_options(regression_tests PRIVATE /UNDEBUG)
    else()
        target_compile_options(nidhogg_tests PRIVATE -UNDEBUG)
        target_compile_options(audit_tests PRIVATE -UNDEBUG)
        target_compile_options(regression_tests PRIVATE -UNDEBUG)
    endif()
endif()

add_custom_target(run_game
    COMMAND $<TARGET_FILE:game_app>
    DEPENDS game_app
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Build and run game_app"
)
