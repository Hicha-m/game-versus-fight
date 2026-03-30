#include "render/assets.h"
#include "render/animation.h"
#include "engine/sdl3_image_compat.h"

#include <stdio.h>
#include <string.h>

#define PATH_BUFFER_SIZE 512

static SDL_Renderer *s_renderer = NULL;
static SDL_Texture *s_textures[ASSET_TEX_COUNT] = {0};

/* Texture paths (PNG for SDL3_image, BMP fallback) */
#if GAME_HAS_SDL3_IMAGE
static const char *texture_rel_paths[ASSET_TEX_COUNT] = {
    "assets/texture/Backgrounds/bForest_0.png",
    "assets/sprites/sFlyingSword/sFlyingSword_0.png",
};
#else
static const char *texture_rel_paths[ASSET_TEX_COUNT] = {
    "assets/texture/Backgrounds/bForest_0.bmp",
    "assets/sprites/sFlyingSword/sFlyingSword_0.bmp",
};
#endif

/* ================================================================ */
/*  File helpers                                                    */
/* ================================================================ */

static bool file_exists(const char *path)
{
    if (path == NULL) return false;
    FILE *file = fopen(path, "rb");
    if (file == NULL) return false;
    fclose(file);
    return true;
}

static bool resolve_asset_path(const char *relative_path, char *out_path, size_t out_size)
{
    if (relative_path == NULL || out_path == NULL || out_size == 0) return false;

    const char *prefixes[] = { "", "../", "../../" };
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
        snprintf(out_path, out_size, "%s%s", prefixes[i], relative_path);
        if (file_exists(out_path)) return true;
    }

    out_path[0] = '\0';
    return false;
}

static SDL_Texture *load_texture_from_path(const char *path)
{
    if (path == NULL || s_renderer == NULL) return NULL;

#if GAME_HAS_SDL3_IMAGE
    return IMG_LoadTexture(s_renderer, path);
#else
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (surface == NULL) return NULL;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(s_renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
#endif
}

/* ================================================================ */
/*  Public API                                                      */
/* ================================================================ */

GameError assets_init(SDL_Renderer *renderer)
{
    if (renderer == NULL) return GAME_ERROR_INVALID_ARGUMENT;

    s_renderer = renderer;
    for (int i = 0; i < ASSET_TEX_COUNT; ++i) {
        s_textures[i] = NULL;
    }

    /* Load all textures */
    for (int i = 0; i < ASSET_TEX_COUNT; ++i) {
        char path[PATH_BUFFER_SIZE] = {0};

        if (!resolve_asset_path(texture_rel_paths[i], path, sizeof(path))) {
            SDL_Log("assets_init: missing texture '%s'", texture_rel_paths[i]);
            continue;
        }

        s_textures[i] = load_texture_from_path(path);
        if (s_textures[i] == NULL) {
            SDL_Log("assets_init: failed loading '%s': %s", path, SDL_GetError());
        }
    }

    /* Initialize animation system */
    if (animation_init(renderer) != GAME_OK) {
        SDL_Log("assets_init: animation_init failed");
        return GAME_ERROR_INITIALIZATION;
    }

    SDL_Log("assets_init: textures and animations loaded");
    return GAME_OK;
}

void assets_shutdown(void)
{
    animation_shutdown();
    for (int i = 0; i < ASSET_TEX_COUNT; ++i) {
        if (s_textures[i] != NULL) {
            SDL_DestroyTexture(s_textures[i]);
            s_textures[i] = NULL;
        }
    }
    s_renderer = NULL;
}

SDL_Texture *assets_get_texture(AssetTextureId id)
{
    if (id < 0 || id >= ASSET_TEX_COUNT) return NULL;
    return s_textures[id];
}
