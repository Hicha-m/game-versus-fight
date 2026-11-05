#include "assets_private.h"
#include "sdl3_image_compat.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#define PATH_BUFFER_SIZE 512

#define ANIMATION_COUNT 13
#define ANIMATION_FRAMES 32
static SDL_Texture *animations[ANIMATION_COUNT][ANIMATION_FRAMES];
static i32 animation_counts[ANIMATION_COUNT];
static SDL_Renderer *s_renderer = NULL;
static SDL_Texture *s_textures[ASSET_TEX_COUNT] = {0};

#if GAME_HAS_SDL3_IMAGE
static const char *texture_rel_paths[ASSET_TEX_COUNT] = {
    "assets/texture/Backgrounds/bForest_0.png",
    "assets/sprites/sManNoWeaponStand/sManNoWeaponStand_0.png",
    "assets/sprites/sManSwordStandMed/sManSwordStandMed_0.png",
    "assets/sprites/sFlyingSword/sFlyingSword_0.png",
};
#else
static const char *texture_rel_paths[ASSET_TEX_COUNT] = {
    "assets/texture/Backgrounds/bForest_0.bmp",
    "assets/sprites/sManNoWeaponStand/sManNoWeaponStand_0.bmp",
    "assets/sprites/sManSwordStandMed/sManSwordStandMed_0.bmp",
    "assets/sprites/sFlyingSword/sFlyingSword_0.bmp",
};
#endif

static bool file_exists(const char *path)
{
    if (path == NULL)
        return false;
    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return false;
    fclose(file);
    return true;
}

static bool resolve_asset_path(const char *relative_path, char *out_path, size_t out_size)
{
    if (relative_path == NULL || out_path == NULL || out_size == 0)
        return false;

    const char *prefixes[] = {"", "../", "../../"};
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i)
    {
        snprintf(out_path, out_size, "%s%s", prefixes[i], relative_path);
        if (file_exists(out_path))
            return true;
    }

    out_path[0] = '\0';
    return false;
}

static SDL_Texture *load_texture_from_path(const char *path)
{
    if (path == NULL || s_renderer == NULL)
        return NULL;

#if GAME_HAS_SDL3_IMAGE
    return IMG_LoadTexture(s_renderer, path);
#else
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (surface == NULL)
        return NULL;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(s_renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
#endif
}

GameError assets_init(SDL_Renderer *renderer)
{
    if (renderer == NULL)
        return GAME_ERROR_INVALID_ARGUMENT;

    s_renderer = renderer;
    for (int i = 0; i < ASSET_TEX_COUNT; ++i)
    {
        s_textures[i] = NULL;
    }

    for (int i = 0; i < ASSET_TEX_COUNT; ++i)
    {
        char path[PATH_BUFFER_SIZE] = {0};

        if (!resolve_asset_path(texture_rel_paths[i], path, sizeof(path)))
        {
            SDL_Log("assets_init: missing texture '%s'", texture_rel_paths[i]);
            continue;
        }

        s_textures[i] = load_texture_from_path(path);
        if (s_textures[i] == NULL)
        {
            SDL_Log("assets_init: failed loading '%s': %s", path, SDL_GetError());
        }
    }

    static const char *names[] = {"sManRun",
                                  "sManJump",
                                  "sManFall",
                                  "sManJumpRoll",
                                  "sManSwordAttackHi",
                                  "sManSwordAttackMed",
                                  "sManSwordAttackLo",
                                  "sManNoWeaponAttackPunch",
                                  "sManDisarmedUpAndBackward",
                                  "sManDeadFall",
                                  "sManThrowStanding",
                                  "sManSwordStandHi",
                                  "sManSwordStandLo"};
    for (i32 a = 0; a < ANIMATION_COUNT; a++)
        for (i32 frame = 0; frame < ANIMATION_FRAMES; frame++)
        {
            char relative[256], path[PATH_BUFFER_SIZE];
            snprintf(relative, sizeof(relative), "assets/sprites/%s/%s_%d.png", names[a], names[a],
                     frame);
            if (!resolve_asset_path(relative, path, sizeof(path)))
                break;
            animations[a][frame] = load_texture_from_path(path);
            if (!animations[a][frame])
                break;
            animation_counts[a]++;
        }
    SDL_Log("assets_init: textures loaded");
    return GAME_OK;
}

void assets_shutdown(void)
{
    for (int i = 0; i < ASSET_TEX_COUNT; ++i)
    {
        if (s_textures[i] != NULL)
        {
            SDL_DestroyTexture(s_textures[i]);
            s_textures[i] = NULL;
        }
    }
    for (i32 a = 0; a < ANIMATION_COUNT; a++)
    {
        for (i32 f = 0; f < animation_counts[a]; f++)
        {
            SDL_DestroyTexture(animations[a][f]);
            animations[a][f] = NULL;
        }
        animation_counts[a] = 0;
    }
    s_renderer = NULL;
}

SDL_Texture *assets_get_texture(AssetTextureId id)
{
    if (id < 0 || id >= ASSET_TEX_COUNT)
        return NULL;
    return s_textures[id];
}

SDL_Texture *assets_animation(i32 animation, f32 time, bool loop)
{
    if (animation < 0 || animation >= ANIMATION_COUNT || !animation_counts[animation])
        return NULL;
    i32 count = animation_counts[animation];
    i32 frame = (i32)(fmaxf(time, 0) * 24.0f);
    frame = loop ? frame % count : (frame < count ? frame : count - 1);
    return animations[animation][frame];
}
