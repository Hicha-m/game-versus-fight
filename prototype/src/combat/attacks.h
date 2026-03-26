#ifndef COMBAT_ATTACKS_H
#define COMBAT_ATTACKS_H

#include "types.h"

/**
 * Attack system with sword heights
 *
 * Nidhogg-like mechanic:
 * - Sword-vs-sword collisions auto-block only on exact matching height.
 * - No manual parry is involved in thrust-vs-thrust resolution.
 * - Any mismatch (HIGH/MID/LOW different) means the attack lane is not blocked.
 */

/**
 * Check if an attack can hit based on sword heights
 * Returns true if attacker can hit defender
 */
bool attacks_can_hit(SwordHeight attacker_height, SwordHeight defender_height);

/**
 * Allocate an attack action
 */
Action attacks_create_action(ActionType type, SwordHeight height, uint32_t frame);

#endif