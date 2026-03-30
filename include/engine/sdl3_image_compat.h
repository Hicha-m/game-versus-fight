#ifndef ENGINE_SDL3_IMAGE_COMPAT_H
#define ENGINE_SDL3_IMAGE_COMPAT_H

#include "core/types.h"

/**
 * SDL3_image compatibility layer.
 * When GAME_HAS_SDL3_IMAGE is defined, uses IMG_LoadTexture from SDL3_image.
 * Otherwise, falls back to SDL_LoadBMP + SDL_CreateTextureFromSurface.
 */

#if GAME_HAS_SDL3_IMAGE

#if defined(GAME_HAS_SDL3_IMAGE_HEADER) && GAME_HAS_SDL3_IMAGE_HEADER
#include <SDL3_image/SDL_image.h>
#else
/* Runtime-only fallback when headers are unavailable */
#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

SDL_Texture *IMG_LoadTexture(SDL_Renderer *renderer, const char *file);

#ifdef __cplusplus
}
#endif

#endif /* GAME_HAS_SDL3_IMAGE_HEADER */

#endif /* GAME_HAS_SDL3_IMAGE */

#endif /* ENGINE_SDL3_IMAGE_COMPAT_H */
