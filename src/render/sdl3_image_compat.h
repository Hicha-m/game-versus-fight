#ifndef ENGINE_SDL3_IMAGE_COMPAT_H
#define ENGINE_SDL3_IMAGE_COMPAT_H

#include "core/types.h"

#if GAME_HAS_SDL3_IMAGE

#if defined(GAME_HAS_SDL3_IMAGE_HEADER) && GAME_HAS_SDL3_IMAGE_HEADER
#include <SDL3_image/SDL_image.h>
#else

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

SDL_Texture *IMG_LoadTexture(SDL_Renderer *renderer, const char *file);

#ifdef __cplusplus
}
#endif

#endif

#endif

#endif
