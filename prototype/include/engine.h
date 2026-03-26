#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include <SDL3/SDL.h>


GameError engine_init(void);
GameError engine_shutdown(void);
SDL_Renderer *engine_renderer_get(void);

#endif
