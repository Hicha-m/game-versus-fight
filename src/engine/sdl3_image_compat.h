#ifndef ENGINE_SDL3_IMAGE_COMPAT_H
#define ENGINE_SDL3_IMAGE_COMPAT_H

#include "types.h"

#if GAME_HAS_SDL3_IMAGE

#if defined(GAME_HAS_SDL3_IMAGE_HEADER) && GAME_HAS_SDL3_IMAGE_HEADER
#include <SDL3_image/SDL_image.h>
#else
#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Runtime-only fallback when SDL3_image headers are unavailable.
 * The symbol is resolved from the linked SDL3_image shared library.
 */
SDL_Texture *IMG_LoadTexture(SDL_Renderer *renderer, const char *file);

#ifdef __cplusplus
}
#endif

#endif

#endif

#endif