#ifndef RENDER_ANIMATION_H
#define RENDER_ANIMATION_H

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "game/combat/combat.h"
#include "core/types.h"

/**
 * Animation state machine for fighter sprites.
 * Derived from FighterAction + combat state.
 */
typedef enum {
    FIGHTER_ANIM_IDLE = 0,
    FIGHTER_ANIM_WALK_FWD,
    FIGHTER_ANIM_WALK_BWD,
    FIGHTER_ANIM_START_RUN,
    FIGHTER_ANIM_RUN,
    FIGHTER_ANIM_SKID,
    FIGHTER_ANIM_ROLL,
    FIGHTER_ANIM_JUMP,
    FIGHTER_ANIM_FALL,
    FIGHTER_ANIM_ATTACK_HI,
    FIGHTER_ANIM_ATTACK_MID,
    FIGHTER_ANIM_ATTACK_LO,
    FIGHTER_ANIM_THROW,
    FIGHTER_ANIM_HIT,
    FIGHTER_ANIM_DEAD,
    FIGHTER_ANIM_COUNT
} FighterAnimState;

/**
 * Initialize animation system with SDL renderer.
 * Loads all fighter sprites from assets/ folder.
 * Gracefully degrades if SDL3_image unavailable (returns NULL textures).
 */
GameError animation_init(SDL_Renderer *renderer);

/** Cleanup all animation textures. */
void animation_shutdown(void);

/**
 * Update animation state for a fighter.
 * Call once per tick for each fighter.
 * player_idx: 0 or 1
 */
void animation_update(int player_idx, const FighterState *fighter);

/** Get body texture for current animation frame. May return NULL. */
SDL_Texture *animation_get_body_texture(int player_idx, const FighterState *fighter);

/** Get sword overlay texture (for armed run/jump/fall). May return NULL. */
SDL_Texture *animation_get_ovl_texture(int player_idx, const FighterState *fighter);

/** Get thrown sword projectile texture from normalized throw lifetime [0..1]. */
SDL_Texture *animation_get_throw_projectile_texture(f32 normalized_time);

/** Check if this state draws sword (returns true for armed idle/walk/attack). */
bool animation_draws_sword(int player_idx, const FighterState *fighter);

/** Get flip mode for facing (SDL_FLIP_HORIZONTAL if facing left, SDL_FLIP_NONE otherwise). */
SDL_FlipMode animation_flip(const FighterState *fighter);

#endif /* RENDER_ANIMATION_H */
