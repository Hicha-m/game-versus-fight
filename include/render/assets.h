#ifndef RENDER_ASSETS_H
#define RENDER_ASSETS_H

#include <SDL3/SDL.h>
#include "core/types.h"

typedef enum {
    ASSET_TEX_BACKGROUND_0 = 0,
    ASSET_TEX_FIGHTER_UNARMED,
    ASSET_TEX_FIGHTER_SWORD,
    ASSET_TEX_FLYING_SWORD,
    ASSET_TEX_COUNT
} AssetTextureId;

GameError assets_init(SDL_Renderer *renderer);

void assets_shutdown(void);

SDL_Texture *assets_get_texture(AssetTextureId id);

#endif
