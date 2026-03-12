#ifndef COMBAT_MOMENTUM_H
#define COMBAT_MOMENTUM_H

#include "types.h"

/**
 * Momentum system (GDD: Passive Mechanic)
 * 
 * "More the player attacks and avances without dying, more their velocity increases progressively"
 * 
 * Pattern:
 * - Increment momentum_frames each frame fighter is alive and attacking
 * - Momentum applies speed bonus: velocity *= (1.0 + momentum_factor)
 * - Reset to 0 on death
 * - Creates "snowball effect" rewarding aggression
 */

float momentum_get_speed_bonus(uint32_t momentum_frames);

#endif