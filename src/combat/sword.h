#ifndef COMBAT_SWORD_H
#define COMBAT_SWORD_H

#include "types.h"

/**
 * Sword evolution system (GDD: Passive Mechanic)
 * 
 * "3 successful parries → Sword reach increases 10%"
 * "Death → Sword reach reverts to normal"
 * 
 * Pattern:
 * - Track successful_parries count in FighterState
 * - Every 3 parries, increment evolution level
 * - Reset to 0 on death
 * - Reach bonus = 1.1 ^ evolution_level
 */

float sword_get_reach_bonus(uint16_t successful_parries);

#endif