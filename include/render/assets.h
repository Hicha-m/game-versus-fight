#ifndef RENDER_ASSETS_H
#define RENDER_ASSETS_H

#include <SDL3/SDL.h>
#include "core/types.h"

/**
 * Asset texture IDs for backgrounds, props, etc.
 */
typedef enum {
    ASSET_TEX_BACKGROUND_0 = 0,
    ASSET_TEX_FLYING_SWORD,
    ASSET_TEX_COUNT
} AssetTextureId;

/**
 * Initialize asset system with SDL renderer.
 * Loads all background textures and sprites.
 */
GameError assets_init(SDL_Renderer *renderer);

/** Cleanup all asset textures. */
void assets_shutdown(void);

/** Get texture by ID. May return NULL if not loaded. */
SDL_Texture *assets_get_texture(AssetTextureId id);

#endif /* RENDER_ASSETS_H */
