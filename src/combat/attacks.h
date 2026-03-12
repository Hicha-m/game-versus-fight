#ifndef COMBAT_ATTACKS_H
#define COMBAT_ATTACKS_H

#include "types.h"

/**
 * Attack system with sword heights
 * 
 * GDD mechanics:
 * - Attack: Frappe avec l'épée
 * - Parry: Bloquer l'attaque
 * - Sword height: High/Mid/Low determines which attacks you block/lose to
 * - Rock-paper-scissors: High blocks High, loses to Low, etc.
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