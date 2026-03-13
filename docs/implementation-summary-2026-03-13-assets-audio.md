# Implementation Summary - 2026-03-13 (Assets + Audio)

## Scope Completed

Implemented end-to-end integration for asset and sound usage in the game loop:

1. Added asset and audio runtime modules.
2. Integrated engine lifecycle hooks (init/shutdown).
3. Wired gameplay/menu sound triggers.
4. Switched renderer to texture-first drawing with fallback.
5. Preserved compatibility when optional image dependency is missing.

## Files Changed

- `CMakeLists.txt`
- `src/engine/audio.h`
- `src/engine/audio.c`
- `src/engine/assets.h`
- `src/engine/assets.c`
- `src/engine/engine.c`
- `src/engine/render.c`
- `src/game/game.c`
- `src/game/match.c`

## Behavior Added

### Audio

- SDL audio subsystem now initialized in `engine_init`.
- WAV SFX loaded from `assets/sound/` with path fallback (``, `../`, `../../`).
- SFX playback added for:
  - menu confirm/back
  - block clash
  - hit
  - pickup
  - death
  - spawn/start of match

### Rendering

- Texture manager introduced (`assets_init`, `assets_get_texture`, `assets_shutdown`).
- `render.c` now attempts to draw:
  - background texture
  - fighter texture
  - sword texture
- If texture unavailable, existing primitive rectangles are still used.

## Environment Wall Encountered

### Missing `SDL3_image` CMake package

Build environment does not provide `SDL3_imageConfig.cmake`, so PNG loading cannot be linked currently.

To avoid blocking the whole project:

- `SDL3_image` is now optional in CMake.
- Compile flag `GAME_HAS_SDL3_IMAGE` controls PNG path.
- Without `SDL3_image`, game still builds and runs with primitive rendering fallback.

## Validation

- Build: success (`game_app`, `game_tests`).
- Test run: all tests pass.

## How To Enable Full PNG Rendering

Install SDL3_image development package, then rebuild.

Typical distro package name examples:
- `SDL3_image`
- `SDL3_image-devel`
- `libsdl3-image-dev`

After installation, rerun build; CMake will detect `SDL3_image` and enable texture loading automatically.
