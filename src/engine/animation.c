#include "animation.h"
#include "sdl3_image_compat.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Tunables                                                            */
/* ------------------------------------------------------------------ */

#define MAX_ANIM_FRAMES   32
#define PATH_BUF_SIZE     512

/* tiles/s — below this threshold horizontal motion is treated as idle */
#define ANIM_MOVE_THRESH  0.3f
/* tiles/s — above this threshold motion is treated as a run */
#define ANIM_RUN_THRESH   4.0f

/* ------------------------------------------------------------------ */
/*  Internal types                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    SDL_Texture *frames[MAX_ANIM_FRAMES];
    uint8_t      frame_count;
    uint8_t      frame_duration; /* game ticks per anim frame */
    bool         loop;
} Anim;

typedef struct {
    FighterAnimState state;
    uint8_t          frame_index;
    uint8_t          frame_timer;
    /* cached from last animation_update() for use by getters */
    bool             has_sword;
    SwordHeight      sword_height;
    bool             prev_attack_held;
    uint8_t          prev_stun_frames;
    float            prev_velocity_x;
} AnimPlayer;

/* ------------------------------------------------------------------ */
/*  Storage                                                             */
/* ------------------------------------------------------------------ */

static SDL_Renderer *s_renderer   = NULL;
static bool          s_initialized = false;

/* Unarmed body animations, indexed by FighterAnimState */
static Anim s_unarmed[FIGHTER_ANIM_COUNT];

/* Armed combined-sprite animations: [FighterAnimState][height: 0=LO 1=MID 2=HI] */
static Anim s_armed[FIGHTER_ANIM_COUNT][3];

/* Sword overlay sprites for armed run/jump/fall (height-independent, stored at [state]) */
static Anim s_sword_ovl[FIGHTER_ANIM_COUNT];

/* Per-fighter runtime playback state */
static AnimPlayer s_players[2];

/* ------------------------------------------------------------------ */
/*  File / path helpers                                                 */
/* ------------------------------------------------------------------ */

static bool path_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static bool resolve_asset_path(const char *rel, char *out, size_t size) {
    static const char *prefixes[] = { "", "../", "../../" };
    for (int i = 0; i < 3; ++i) {
        snprintf(out, size, "%s%s", prefixes[i], rel);
        if (path_exists(out)) return true;
    }
    out[0] = '\0';
    return false;
}

static SDL_Texture *load_texture(const char *path) {
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

/* ------------------------------------------------------------------ */
/*  Animation loading                                                   */
/* ------------------------------------------------------------------ */

/*
 * Load a sprite sequence into *anim.
 * Filenames follow: "assets/sprites/<folder>/<folder>_N.png" (N starting at 0).
 * frame_count: expected number of frames (auto-probes up to MAX_ANIM_FRAMES when 0).
 */
static void load_anim(Anim *anim, const char *folder, int frame_count,
                      uint8_t duration, bool loop) {
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

static void free_anim(Anim *anim) {
    for (int i = 0; i < (int)anim->frame_count; ++i) {
        if (anim->frames[i]) {
            SDL_DestroyTexture(anim->frames[i]);
            anim->frames[i] = NULL;
        }
    }
    anim->frame_count = 0;
}

/* ------------------------------------------------------------------ */
/*  State derivation from FighterState                                  */
/* ------------------------------------------------------------------ */

static FighterAnimState attack_state_for_height(SwordHeight height) {
    switch (height) {
    case SWORD_HEIGHT_HIGH: return FIGHTER_ANIM_ATTACK_HI;
    case SWORD_HEIGHT_LOW:  return FIGHTER_ANIM_ATTACK_LO;
    case SWORD_HEIGHT_MID:
    default:
        return FIGHTER_ANIM_ATTACK_MID;
    }
}

static FighterAnimState derive_state(const FighterState *f) {
    if (!f->alive || f->downed) return FIGHTER_ANIM_DEAD;

    if (!f->grounded) {
        return (f->velocity.y <= 0.0f) ? FIGHTER_ANIM_JUMP : FIGHTER_ANIM_FALL;
    }

    float vx = fabsf(f->velocity.x);
    if (vx > ANIM_MOVE_THRESH) {
        bool moving_forward = (f->facing == FACING_RIGHT) == (f->velocity.x > 0.0f);
        if (vx > ANIM_RUN_THRESH) {
            return moving_forward ? FIGHTER_ANIM_RUN : FIGHTER_ANIM_WALK_BWD;
        }
        return moving_forward ? FIGHTER_ANIM_WALK_FWD : FIGHTER_ANIM_WALK_BWD;
    }

    return FIGHTER_ANIM_IDLE;
}

static bool is_one_shot_state(FighterAnimState state) {
    return state == FIGHTER_ANIM_ATTACK_HI ||
           state == FIGHTER_ANIM_ATTACK_MID ||
           state == FIGHTER_ANIM_ATTACK_LO ||
           state == FIGHTER_ANIM_HIT ||
           state == FIGHTER_ANIM_START_RUN ||
           state == FIGHTER_ANIM_SKID;
}

static bool is_anim_finished(const Anim *anim, uint8_t frame_index) {
    if (anim == NULL || anim->frame_count == 0) {
        return true;
    }
    if (anim->loop) {
        return false;
    }
    return frame_index >= (uint8_t)(anim->frame_count - 1);
}

/* ------------------------------------------------------------------ */
/*  Animation selection                                                 */
/* ------------------------------------------------------------------ */

static int height_index(SwordHeight h) {
    switch (h) {
    case SWORD_HEIGHT_HIGH: return 2;
    case SWORD_HEIGHT_MID:  return 1;
    case SWORD_HEIGHT_LOW:
    default:                return 0;
    }
}

/*
 * Pick the body animation for the given player's current state.
 * Never returns NULL; may return an Anim with frame_count == 0
 * (caller should treat that as "no texture available").
 */
static const Anim *select_body_anim(int pidx, bool has_sword, SwordHeight height) {
    FighterAnimState st = s_players[pidx].state;
    int hi = height_index(height);

    if (!has_sword) {
        const Anim *a = &s_unarmed[st];
        return (a->frame_count > 0) ? a : &s_unarmed[FIGHTER_ANIM_IDLE];
    }

    /* For run / jump / fall / hit / dead: use unarmed body; sword shown via overlay */
    if (st == FIGHTER_ANIM_START_RUN || st == FIGHTER_ANIM_RUN ||
        st == FIGHTER_ANIM_SKID || st == FIGHTER_ANIM_ROLL ||
        st == FIGHTER_ANIM_CARTWHEEL || st == FIGHTER_ANIM_JUMP ||
        st == FIGHTER_ANIM_FALL || st == FIGHTER_ANIM_HIT  ||
        st == FIGHTER_ANIM_DEAD) {
        const Anim *a = &s_unarmed[st];
        return (a->frame_count > 0) ? a : &s_unarmed[FIGHTER_ANIM_IDLE];
    }

    /* Combined sManSword* sprite for idle / walk / attack states */
    const Anim *a = &s_armed[st][hi];
    if (a->frame_count > 0) return a;

    /* height fallback: try other heights */
    for (int h2 = 0; h2 < 3; ++h2) {
        if (s_armed[st][h2].frame_count > 0) return &s_armed[st][h2];
    }

    /* ultimate fallback: unarmed idle */
    const Anim *fb = &s_unarmed[FIGHTER_ANIM_IDLE];
    return (fb->frame_count > 0) ? fb : &s_unarmed[st];
}

static const Anim *select_ovl_anim(int pidx, bool has_sword) {
    if (!has_sword) return NULL;
    FighterAnimState st = s_players[pidx].state;
    if (st != FIGHTER_ANIM_START_RUN && st != FIGHTER_ANIM_RUN &&
        st != FIGHTER_ANIM_SKID && st != FIGHTER_ANIM_ROLL &&
        st != FIGHTER_ANIM_CARTWHEEL && st != FIGHTER_ANIM_JUMP &&
        st != FIGHTER_ANIM_FALL) {
        return NULL;
    }
    const Anim *a = &s_sword_ovl[st];
    return (a->frame_count > 0) ? a : NULL;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

GameError animation_init(SDL_Renderer *renderer) {
    if (!renderer) return GAME_ERROR_INVALID_ARGUMENT;
    s_renderer = renderer;
    memset(s_unarmed,   0, sizeof(s_unarmed));
    memset(s_armed,     0, sizeof(s_armed));
    memset(s_sword_ovl, 0, sizeof(s_sword_ovl));
    memset(s_players,   0, sizeof(s_players));

    /* ---- Unarmed body animations ---- */
    load_anim(&s_unarmed[FIGHTER_ANIM_IDLE],       "sManNoWeaponStand",       17, 4, true );
    load_anim(&s_unarmed[FIGHTER_ANIM_WALK_FWD],   "sManNoWeaponForward",      1, 6, true );
    load_anim(&s_unarmed[FIGHTER_ANIM_WALK_BWD],   "sManNoWeaponBackward",     1, 6, true );
    load_anim(&s_unarmed[FIGHTER_ANIM_START_RUN],  "sManStartRunning",        24, 2, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_RUN],        "sManRun",                 24, 3, true );
    load_anim(&s_unarmed[FIGHTER_ANIM_SKID],       "sManSkid",                 2, 3, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_ROLL],       "sManRollLandStick",        4, 2, true );
    load_anim(&s_unarmed[FIGHTER_ANIM_CARTWHEEL],  "sManBackflip",            11, 2, true );
    load_anim(&s_unarmed[FIGHTER_ANIM_JUMP],       "sManJump",                 3, 5, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_FALL],       "sManFall",                10, 4, true );
    load_anim(&s_unarmed[FIGHTER_ANIM_ATTACK_HI],  "sManNoWeaponAttackPunch", 13, 3, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_ATTACK_MID], "sManNoWeaponAttackPunch", 13, 3, false);
    load_anim(&s_unarmed[FIGHTER_ANIM_ATTACK_LO],  "sManNoWeaponAttackPunch", 13, 3, false);
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

    /* ---- Armed attack (each height gets its own sprites; all three heights loaded per slot) ---- */
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_LO][0],  "sManSwordAttackLo",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_LO][1],  "sManSwordAttackLo",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_LO][2],  "sManSwordAttackLo",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_MID][0], "sManSwordAttackMed", 15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_MID][1], "sManSwordAttackMed", 15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_MID][2], "sManSwordAttackMed", 15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_HI][0],  "sManSwordAttackHi",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_HI][1],  "sManSwordAttackHi",  15, 3, false);
    load_anim(&s_armed[FIGHTER_ANIM_ATTACK_HI][2],  "sManSwordAttackHi",  15, 3, false);

    /* ---- Sword overlays for armed movement (height-independent) ---- */
    load_anim(&s_sword_ovl[FIGHTER_ANIM_START_RUN], "sSwordStartRunning", 1, 2, false);
    load_anim(&s_sword_ovl[FIGHTER_ANIM_RUN],  "sSwordRun",  24, 3, true );
    load_anim(&s_sword_ovl[FIGHTER_ANIM_SKID], "sSwordSkid", 2, 3, false);
    load_anim(&s_sword_ovl[FIGHTER_ANIM_ROLL], "sSwordRoll", 5, 2, true );
    load_anim(&s_sword_ovl[FIGHTER_ANIM_CARTWHEEL], "sSwordBackflip", 2, 2, true);
    load_anim(&s_sword_ovl[FIGHTER_ANIM_FALL], "sSwordFall", 10, 4, true );
    load_anim(&s_sword_ovl[FIGHTER_ANIM_JUMP], "sSwordJump",  1, 5, false);

    s_initialized = true;
    SDL_Log("animation_init: fighter sprites loaded");
    return GAME_OK;
}

void animation_shutdown(void) {
    if (!s_initialized) return;
    for (int i = 0; i < (int)FIGHTER_ANIM_COUNT; ++i) {
        free_anim(&s_unarmed[i]);
        free_anim(&s_sword_ovl[i]);
        for (int h = 0; h < 3; ++h) {
            free_anim(&s_armed[i][h]);
        }
    }
    s_initialized = false;
    s_renderer = NULL;
}

void animation_update(int player_idx, const FighterState *fighter) {
    if (player_idx < 0 || player_idx >= 2 || !fighter || !s_initialized) return;

    AnimPlayer       *p         = &s_players[player_idx];
    FighterAnimState  base_state = derive_state(fighter);
    FighterAnimState  new_state = base_state;
    bool attack_started = fighter->attack_was_held && !p->prev_attack_held;
    bool hit_started = fighter->stun_frames > 0 && p->prev_stun_frames == 0;
    float abs_vx = fabsf(fighter->velocity.x);
    float prev_abs_vx = fabsf(p->prev_velocity_x);
    bool started_run = fighter->grounded && prev_abs_vx <= ANIM_MOVE_THRESH && abs_vx > ANIM_RUN_THRESH;
    bool skid_started = fighter->grounded && prev_abs_vx > ANIM_RUN_THRESH && abs_vx <= ANIM_MOVE_THRESH;

    /* Cache fighter fields for getters called later this frame */
    p->has_sword   = fighter->has_sword && fighter->sword_ready;
    p->sword_height = fighter->sword_height;

    if (!fighter->alive || fighter->downed) {
        new_state = FIGHTER_ANIM_DEAD;
    } else if (fighter->cartwheeling) {
        new_state = FIGHTER_ANIM_CARTWHEEL;
    } else if (fighter->rolling) {
        new_state = FIGHTER_ANIM_ROLL;
    } else if (hit_started) {
        new_state = FIGHTER_ANIM_HIT;
    } else if (attack_started) {
        new_state = attack_state_for_height(fighter->sword_height);
    } else if (skid_started) {
        new_state = FIGHTER_ANIM_SKID;
    } else if (started_run) {
        new_state = FIGHTER_ANIM_START_RUN;
    } else if (is_one_shot_state(p->state)) {
        const Anim *current = select_body_anim(player_idx, p->has_sword, p->sword_height);
        if (!is_anim_finished(current, p->frame_index)) {
            new_state = p->state;
        }
    }

    if (new_state != p->state) {
        /* State changed — restart from frame 0 */
        p->state       = new_state;
        p->frame_index = 0;
        p->frame_timer = 0;
    } else {
        /* Advance frame timer */
        const Anim *body = select_body_anim(player_idx, fighter->has_sword && fighter->sword_ready, fighter->sword_height);
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

    p->prev_attack_held = fighter->attack_was_held;
    p->prev_stun_frames = fighter->stun_frames;
    p->prev_velocity_x = fighter->velocity.x;
}

SDL_Texture *animation_get_body_texture(int player_idx, const FighterState *fighter) {
    if (player_idx < 0 || player_idx >= 2 || !fighter || !s_initialized) return NULL;
    const Anim *a = select_body_anim(player_idx, fighter->has_sword && fighter->sword_ready, fighter->sword_height);
    if (!a || a->frame_count == 0) return NULL;
    uint8_t idx = s_players[player_idx].frame_index;
    if (idx >= a->frame_count) idx = (uint8_t)(a->frame_count - 1);
    return a->frames[idx];
}

SDL_Texture *animation_get_ovl_texture(int player_idx, const FighterState *fighter) {
    if (player_idx < 0 || player_idx >= 2 || !fighter || !s_initialized) return NULL;
    const Anim *a = select_ovl_anim(player_idx, fighter->has_sword && fighter->sword_ready);
    if (!a || a->frame_count == 0) return NULL;
    uint8_t idx = s_players[player_idx].frame_index;
    if (idx >= a->frame_count) idx = (uint8_t)(a->frame_count - 1);
    return a->frames[idx];
}

bool animation_draws_sword(int player_idx, const FighterState *fighter) {
    if (player_idx < 0 || player_idx >= 2 || !fighter) return false;
    if (!fighter->has_sword || !fighter->sword_ready) return false;
    FighterAnimState st = s_players[player_idx].state;
    /* Combined sManSword* sprites include the sword visual */
    return st == FIGHTER_ANIM_IDLE      ||
           st == FIGHTER_ANIM_WALK_FWD  ||
           st == FIGHTER_ANIM_WALK_BWD  ||
           st == FIGHTER_ANIM_ATTACK_HI ||
           st == FIGHTER_ANIM_ATTACK_MID||
           st == FIGHTER_ANIM_ATTACK_LO;
}

SDL_FlipMode animation_flip(const FighterState *fighter) {
    return (fighter && fighter->facing == FACING_LEFT)
        ? SDL_FLIP_HORIZONTAL
        : SDL_FLIP_NONE;
}
