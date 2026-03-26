#ifndef ENGINE_ANIMATION_H
#define ENGINE_ANIMATION_H

#include "types.h"
#include <SDL3/SDL.h>

/**
 * Logical animation states derived from FighterState physics fields.
 * The renderer uses these (together with has_sword / sword_height) to
 * pick the right sprite sequence.
 */
typedef enum FighterAnimState {
    FIGHTER_ANIM_IDLE = 0,
    FIGHTER_ANIM_WALK_FWD,
    FIGHTER_ANIM_WALK_BWD,
    FIGHTER_ANIM_START_RUN,
    FIGHTER_ANIM_RUN,
    FIGHTER_ANIM_SKID,
    FIGHTER_ANIM_ROLL,
    FIGHTER_ANIM_CARTWHEEL,
    FIGHTER_ANIM_JUMP,
    FIGHTER_ANIM_FALL,
    FIGHTER_ANIM_ATTACK_HI,
    FIGHTER_ANIM_ATTACK_MID,
    FIGHTER_ANIM_ATTACK_LO,
    FIGHTER_ANIM_HIT,
    FIGHTER_ANIM_DEAD,
    FIGHTER_ANIM_COUNT
} FighterAnimState;

/**
 * Load all fighter sprite sequences.
 * Must be called after the SDL renderer is created.
 * Fails gracefully when SDL3_image is unavailable — all textures stay NULL
 * and the game falls back to solid-rectangle rendering.
 */
GameError animation_init(SDL_Renderer *renderer);

/** Free every animation texture. */
void animation_shutdown(void);

/**
 * Advance the per-fighter animation state machine.
 * Call once per game tick for each fighter.
 * player_idx: 0 = PLAYER_ONE, 1 = PLAYER_TWO.
 */
void animation_update(int player_idx, const FighterState *fighter);

/**
 * Return the body texture for the current animation frame.
 * May return NULL when no texture is loaded (rectangle fallback).
 */
SDL_Texture *animation_get_body_texture(int player_idx, const FighterState *fighter);

/**
 * Return the sword-overlay texture for the current frame (armed run/jump/fall).
 * Returns NULL for all other states or when the overlay is unavailable.
 */
SDL_Texture *animation_get_ovl_texture(int player_idx, const FighterState *fighter);

/**
 * Returns true when the current body sprite already contains the sword visually
 * (combined sManSword* sprites for idle / walk / attack states).
 * When true the caller should NOT draw the sword line separately.
 */
bool animation_draws_sword(int player_idx, const FighterState *fighter);

/**
 * Returns SDL_FLIP_HORIZONTAL when the fighter faces left, SDL_FLIP_NONE otherwise.
 */
SDL_FlipMode animation_flip(const FighterState *fighter);

#endif /* ENGINE_ANIMATION_H */
