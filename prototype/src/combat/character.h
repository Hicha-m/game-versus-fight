#ifndef COMBAT_CHARACTER_H
#define COMBAT_CHARACTER_H

#include "types.h"

/**
 * Character/Fighter state management
 * 
 * Manages:
 * - Position and velocity (physics)
 * - Sword height and facing direction
 * - Momentum state
 * - Invulnerability and stun frames
 */

void character_init(FighterState *fighter, float spawn_x, float spawn_y);
void character_update(FighterState *fighter, const PlayerCommand *input, uint32_t dt_ms);
void character_apply_knockback(FighterState *fighter, float force_x, float force_y);
void character_kill(FighterState *fighter);

#endif