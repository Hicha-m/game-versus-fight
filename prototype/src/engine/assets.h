#ifndef ENGINE_ASSETS_H
#define ENGINE_ASSETS_H

#include "types.h"
#include <SDL3/SDL.h>

typedef enum AssetTextureId {
	ASSET_TEX_BACKGROUND,
	ASSET_TEX_FIGHTER,
	ASSET_TEX_SWORD,
	ASSET_TEX_COUNT
} AssetTextureId;

GameError assets_init(SDL_Renderer *renderer);
void assets_shutdown(void);
SDL_Texture *assets_get_texture(AssetTextureId id);

#endif