#include "render/animation.h"
#include "engine/sdl3_image_compat.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ================================================================ */
/*  Tunables                                                        */
/* ================================================================ */

#define MAX_ANIM_FRAMES   32
#define PATH_BUF_SIZE     512

/* tiles/s threshold for walk vs idle */
#define ANIM_MOVE_THRESH  0.3f
/* tiles/s threshold for run vs walk */
#define ANIM_RUN_THRESH   4.0f

/* ================================================================ */
/*  Animation frame data                                            */
/* ================================================================ */

typedef struct {
    SDL_Texture *frames[MAX_ANIM_FRAMES];
    uint8_t      frame_count;
    uint8_t      frame_duration; /* game ticks per frame */
    bool         loop;
} AnimSequence;

typedef struct {
    FighterAnimState state;
    uint8_t          frame_index;
    uint8_t          frame_timer;
    
    /* Cached state from last update */
    bool             has_sword;
    SwordLine        sword_line;
    bool             was_attacking;
    bool             was_stunned;
    f32              prev_velocity_x;
} AnimPlayer;

/* ================================================================ */
/*  Storage                                                         */
/* ================================================================ */

static SDL_Renderer *s_renderer = NULL;
static bool s_initialized = false;

/* Unarmed body animations [FighterAnimState] */
static AnimSequence s_unarmed[FIGHTER_ANIM_COUNT];

/* Armed combined-sprite animations [FighterAnimState][height: 0=LO 1=MID 2=HI] */
static AnimSequence s_armed[FIGHTER_ANIM_COUNT][3];

/* Sword overlay animations for armed movement [FighterAnimState] */
static AnimSequence s_sword_ovl[FIGHTER_ANIM_COUNT];

/* Projectile animation for thrown sword */
static AnimSequence s_throw_projectile;

/* Per-fighter playback state */
static AnimPlayer s_players[2];

/* ================================================================ */
/*  File and path helpers                                           */
/* ================================================================ */

static bool path_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static bool resolve_asset_path(const char *rel, char *out, size_t size)
{
    static const char *prefixes[] = { "", "../", "../../" };
    for (int i = 0; i < 3; ++i) {
        snprintf(out, size, "%s%s", prefixes[i], rel);
        if (path_exists(out)) return true;
    }
    out[0] = '\0';
    return false;
}

static SDL_Texture *load_texture(const char *path)
{
#if GAME_HAS_SDL3_IMAGE
    return IMG_LoadTexture(s_renderer, path);
#else
    SDL_Surface *surf = SDL_LoadBMP(path);
    if (!surf) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(s_renderer, surf);
    SDL_DestroySurface(surf);
    return tex;
#endif
}

/* ================================================================ */
/*  Animation loading                                               */
/* ================================================================ */

static void load_anim(AnimSequence *anim, const char *folder, int frame_count,
                      uint8_t duration, bool loop)
{
    memset(anim, 0, sizeof(*anim));
    anim->frame_duration = (duration > 0) ? duration : 4;
    anim->loop = loop;

    if (frame_count <= 0 || frame_count > MAX_ANIM_FRAMES) {
        frame_count = MAX_ANIM_FRAMES;
    }

    int loaded = 0;
    for (int i = 0; i < frame_count; ++i) {
        char rel[PATH_BUF_SIZE];
        char path[PATH_BUF_SIZE];

#if GAME_HAS_SDL3_IMAGE
        snprintf(rel, sizeof(rel), "assets/sprites/%s/%s_%d.png", folder, folder, i);
#else
        snprintf(rel, sizeof(rel), "assets/sprites/%s/%s_%d.bmp", folder, folder, i);
#endif

        if (!resolve_asset_path(rel, path, sizeof(path))) break;
        
        SDL_Texture *tex = load_texture(path);
        if (!tex) {
            SDL_Log("animation: failed '%s': %s", path, SDL_GetError());
            break;
        }
        anim->frames[loaded++] = tex;
    }
    anim->frame_count = (uint8_t)loaded;
}

static void free_anim(AnimSequence *anim)
{
    for (int i = 0; i < (int)anim->frame_count; ++i) {
        if (anim->frames[i]) {
            SDL_DestroyTexture(anim->frames[i]);
            anim->frames[i] = NULL;
        }
    }
    anim->frame_count = 0;
}

/* ================================================================ */
/*  State derivation from FighterState + FighterAction             */
/* ================================================================ */

static FighterAnimState action_to_anim_state(FighterAction action)
{
    switch (action) {
    case FIGHTER_ACTION_IDLE:         return FIGHTER_ANIM_IDLE;
    case FIGHTER_ACTION_RUN:          return FIGHTER_ANIM_RUN;
    case FIGHTER_ACTION_JUMP:         return FIGHTER_ANIM_JUMP;
    case FIGHTER_ACTION_FALL:         return FIGHTER_ANIM_FALL;
    case FIGHTER_ACTION_ATTACK_HIGH:  return FIGHTER_ANIM_ATTACK_HI;
    case FIGHTER_ACTION_ATTACK_MID:   return FIGHTER_ANIM_ATTACK_MID;
    case FIGHTER_ACTION_ATTACK_LOW:   return FIGHTER_ANIM_ATTACK_LO;
    case FIGHTER_ACTION_THROW:        return FIGHTER_ANIM_THROW;
    case FIGHTER_ACTION_ROLL:         return FIGHTER_ANIM_ROLL;
    case FIGHTER_ACTION_STUN:         return FIGHTER_ANIM_HIT;
    case FIGHTER_ACTION_DEAD:         return FIGHTER_ANIM_DEAD;
    default:                          return FIGHTER_ANIM_IDLE;
    }
}

static FighterAnimState sword_height_to_index(SwordLine line)
{
    switch (line) {
    case SWORD_LINE_HIGH: return 2;
    case SWORD_LINE_MID:  return 1;
    case SWORD_LINE_LOW:
    default:              return 0;
    }
}

static bool is_one_shot_state(FighterAnimState state)
{
    return state == FIGHTER_ANIM_ATTACK_HI ||
           state == FIGHTER_ANIM_ATTACK_MID ||
           state == FIGHTER_ANIM_ATTACK_LO ||
            state == FIGHTER_ANIM_THROW ||
           state == FIGHTER_ANIM_HIT;
}

static bool is_anim_finished(const AnimSequence *anim, uint8_t frame_index)
{
    if (anim == NULL || anim->frame_count == 0) return true;
    if (anim->loop) return false;
    return frame_index >= (uint8_t)(anim->frame_count - 1);
}

/* ================================================================ */
/*  Animation selection                                             */
/* ================================================================ */

static const AnimSequence *select_body_anim(int pidx, bool has_sword, SwordLine line)
{
    FighterAnimState st = s_players[pidx].state;
    int hi = sword_height_to_index(line);

    if (!has_sword) {
        const AnimSequence *a = &s_unarmed[st];
        return (a->frame_count > 0) ? a : &s_unarmed[FIGHTER_ANIM_IDLE];
    }

    /* Armed: use unarmed body for movement states, armed combined sprites for attacks */
    if (st == FIGHTER_ANIM_RUN || st == FIGHTER_ANIM_JUMP ||
        st == FIGHTER_ANIM_FALL || st == FIGHTER_ANIM_THROW || st == FIGHTER_ANIM_HIT ||
        st == FIGHTER_ANIM_DEAD || st == FIGHTER_ANIM_ROLL) {
        const AnimSequence *a = &s_unarmed[st];
        return (a->frame_count > 0) ? a : &s_unarmed[FIGHTER_ANIM_IDLE];
    }

    /* Combined sManSword* sprites for idle/walk/attack states */
    const AnimSequence *a = &s_armed[st][hi];
    if (a->frame_count > 0) return a;

    /* Height fallback */
    for (int h2 = 0; h2 < 3; ++h2) {
        if (s_armed[st][h2].frame_count > 0) return &s_armed[st][h2];
    }

    /* Ultimate fallback */
    const AnimSequence *fb = &s_unarmed[FIGHTER_ANIM_IDLE];
    return (fb->frame_count > 0) ? fb : &s_unarmed[st];
}

static const AnimSequence *select_ovl_anim(int pidx, bool has_sword)
{
    if (!has_sword) return NULL;
    
    FighterAnimState st = s_players[pidx].state;
    if (st != FIGHTER_ANIM_RUN && st != FIGHTER_ANIM_JUMP &&
        st != FIGHTER_ANIM_FALL && st != FIGHTER_ANIM_ROLL) {
        return NULL;
    }
    
    const AnimSequence *a = &s_sword_ovl[st];
    return (a->frame_count > 0) ? a : NULL;
}

/* ================================================================ */
/*  Public API                                                      */
/* ================================================================ */

GameError animation_init(SDL_Renderer *renderer)
{
    if (!renderer) return GAME_ERROR_INVALID_ARGUMENT;
    
    s_renderer = renderer;
    memset(s_unarmed, 0, sizeof(s_unarmed));
    memset(s_armed, 0, sizeof(s_armed));
    memset(s_sword_ovl, 0, sizeof(s_sword_ovl));
    memset(&s_throw_projectile, 0, sizeof(s_throw_projectile));
    memset(s_players, 0, sizeof(s_players));

    /* ---- Unarmed body animations ---- */
    load_anim(&s_unarmed[FIGHTER_ANIM_IDLE],       "sManNoWeaponStand",       17, 4, true);
    load_anim(&s_unarmed[FIGHTER_ANIM_WALK_FWD],   "sManNoWeaponForward",      1, 6, true);
    load_anim(&s_unarmed[FIGHTER_ANIM_WALK_BWD],   "sManNoWeaponBackward",     1, 6, true);
    load_anim(&s_unarmed[FIGHTER_ANIM_RUN],        "sManRun",                 24, 3, true);
    load_anim(&s_unarmed[FIGHTER_ANIM_ROLL],       "sManRollLandStick",        4, 2, true);
    load_anim(&s_unarmed[FIGHTER_ANIM_JUMP],       "sManJump",                 3, 5, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_FALL],       "sManFall",                10, 4, true);
    load_anim(&s_unarmed[FIGHTER_ANIM_ATTACK_HI],  "sManNoWeaponAttackPunch", 13, 3, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_ATTACK_MID], "sManNoWeaponAttackPunch", 13, 3, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_ATTACK_LO],  "sManNoWeaponAttackPunch", 13, 3, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_THROW],      "sManThrowStanding",      13, 2, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_HIT],        "sManHitMed",              12, 3, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_DEAD],       "sManCollapse",            11, 5, false);

    /* ---- Armed idle [LO=0, MID=1, HI=2] ---- */
    load_anim(&s_armed[FIGHTER_ANIM_IDLE][0], "sManSwordStandLo",  12, 5, true);
    load_anim(&s_armed[FIGHTER_ANIM_IDLE][1], "sManSwordStandMed", 12, 5, true);
    load_anim(&s_armed[FIGHTER_ANIM_IDLE][2], "sManSwordStandHi",  10, 5, true);

    /* ---- Armed walk forward ---- */
    load_anim(&s_armed[FIGHTER_ANIM_WALK_FWD][0], "sManSwordForwardLo",  1, 5, true);
    load_anim(&s_armed[FIGHTER_ANIM_WALK_FWD][1], "sManSwordForwardMed", 5, 5, true);
    load_anim(&s_armed[FIGHTER_ANIM_WALK_FWD][2], "sManSwordForwardHi",  5, 5, true);

    /* ---- Armed walk backward ---- */
    load_anim(&s_armed[FIGHTER_ANIM_WALK_BWD][0], "sManSwordBackwardLo",  1, 5, true);
    load_anim(&s_armed[FIGHTER_ANIM_WALK_BWD][1], "sManSwordBackwardMed", 1, 5, true);
    load_anim(&s_armed[FIGHTER_ANIM_WALK_BWD][2], "sManSwordBackwardHi",  4, 5, true);

    /* ---- Armed attack ---- */
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_LO][0],  "sManSwordAttackLo",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_LO][1],  "sManSwordAttackLo",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_LO][2],  "sManSwordAttackLo",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_MID][0], "sManSwordAttackMed", 15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_MID][1], "sManSwordAttackMed", 15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_MID][2], "sManSwordAttackMed", 15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_HI][0],  "sManSwordAttackHi",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_HI][1],  "sManSwordAttackHi",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_HI][2],  "sManSwordAttackHi",  15, 3, false);

    /* ---- Sword overlays for armed movement ---- */
    load_anim(&s_sword_ovl[FIGHTER_ANIM_RUN],   "sSwordRun",   24, 3, true);
    load_anim(&s_sword_ovl[FIGHTER_ANIM_JUMP],  "sSwordJump",   1, 5, false);
    load_anim(&s_sword_ovl[FIGHTER_ANIM_FALL],  "sSwordFall",  10, 4, true);
    load_anim(&s_sword_ovl[FIGHTER_ANIM_ROLL],  "sSwordRoll",   5, 2, true);

    /* Projectile frames for thrown sword */
    load_anim(&s_throw_projectile, "sSwordThrowStanding", 13, 1, true);

    s_initialized = true;
    SDL_Log("animation_init: fighter sprites loaded");
    return GAME_OK;
}

void animation_shutdown(void)
{
    if (!s_initialized) return;
    
    for (int i = 0; i < (int)FIGHTER_ANIM_COUNT; ++i) {
        free_anim(&s_unarmed[i]);
        free_anim(&s_sword_ovl[i]);
        for (int h = 0; h < 3; ++h) {
            free_anim(&s_armed[i][h]);
        }
    }
    free_anim(&s_throw_projectile);
    s_initialized = false;
    s_renderer = NULL;
}

void animation_update(int player_idx, const FighterState *fighter)
{
    if (player_idx < 0 || player_idx >= 2 || !fighter || !s_initialized) return;

    AnimPlayer *p = &s_players[player_idx];
    
    /* Derive animation state from fighter action + combat state */
    FighterAnimState new_state = action_to_anim_state(fighter->action);
    
    /* Cache fighter state for getters */
    p->has_sword = fighter->has_sword;
    p->sword_line = fighter->sword_line;

    /* State transitions */
    if (new_state != p->state) {
        p->state = new_state;
        p->frame_index = 0;
        p->frame_timer = 0;
    } else {
        /* Advance frame in current state */
        const AnimSequence *body = select_body_anim(player_idx, fighter->has_sword, fighter->sword_line);
        uint8_t dur = (body && body->frame_duration > 0) ? body->frame_duration : 4;

        if (++p->frame_timer >= dur) {
            p->frame_timer = 0;
            if (body && body->frame_count > 0) {
                uint8_t next = p->frame_index + 1;
                if (next < body->frame_count) {
                    p->frame_index = next;
                } else if (body->loop) {
                    p->frame_index = 0;
                }
            }
        }
    }

    p->was_attacking = fighter->is_attacking;
    p->was_stunned = fighter->is_stunned;
    p->prev_velocity_x = fighter->vel.x;
}

SDL_Texture *animation_get_body_texture(int player_idx, const FighterState *fighter)
{
    if (player_idx < 0 || player_idx >= 2 || !fighter || !s_initialized) return NULL;
    
    const AnimSequence *a = select_body_anim(player_idx, fighter->has_sword, fighter->sword_line);
    if (!a || a->frame_count == 0) return NULL;
    
    uint8_t idx = s_players[player_idx].frame_index;
    if (idx >= a->frame_count) idx = (uint8_t)(a->frame_count - 1);
    
    return a->frames[idx];
}

SDL_Texture *animation_get_ovl_texture(int player_idx, const FighterState *fighter)
{
    if (player_idx < 0 || player_idx >= 2 || !fighter || !s_initialized) return NULL;
    
    const AnimSequence *a = select_ovl_anim(player_idx, fighter->has_sword);
    if (!a || a->frame_count == 0) return NULL;
    
    uint8_t idx = s_players[player_idx].frame_index;
    if (idx >= a->frame_count) idx = (uint8_t)(a->frame_count - 1);
    
    return a->frames[idx];
}

SDL_Texture *animation_get_throw_projectile_texture(f32 normalized_time)
{
    uint8_t idx;

    if (!s_initialized || s_throw_projectile.frame_count == 0) {
        return NULL;
    }

    normalized_time = (normalized_time < 0.0f) ? 0.0f : normalized_time;
    normalized_time = (normalized_time > 1.0f) ? 1.0f : normalized_time;
    idx = (uint8_t)(normalized_time * (f32)(s_throw_projectile.frame_count - 1));
    if (idx >= s_throw_projectile.frame_count) {
        idx = (uint8_t)(s_throw_projectile.frame_count - 1);
    }
    return s_throw_projectile.frames[idx];
}

bool animation_draws_sword(int player_idx, const FighterState *fighter)
{
    if (player_idx < 0 || player_idx >= 2 || !fighter || !s_initialized) return false;
    if (!fighter->has_sword) return false;
    
    FighterAnimState st = s_players[player_idx].state;
    return st == FIGHTER_ANIM_IDLE      ||
           st == FIGHTER_ANIM_WALK_FWD  ||
           st == FIGHTER_ANIM_WALK_BWD  ||
           st == FIGHTER_ANIM_ATTACK_HI ||
           st == FIGHTER_ANIM_ATTACK_MID||
           st == FIGHTER_ANIM_ATTACK_LO;
}

SDL_FlipMode animation_flip(const FighterState *fighter)
{
    if (!fighter) return SDL_FLIP_NONE;
    return fighter->facing_right ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
}
