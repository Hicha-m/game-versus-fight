#include "combat/attacks.h"

/**
 * Rock-paper-scissors height matchups
 * 
 * From architecture/GDD: "Hauteur épée + Parer crée un système rock-paper-scissors"
 * 
 * Matchup logic:
 * - HIGH blocks HIGH, loses to LOW (attacker at low can hit defender at high)
 * - MID loses to HIGH and LOW (neutral, always vulnerable to extremes)
 * - LOW blocks LOW, loses to HIGH
 * 
 * This creates tactical depth: anticipate opponent's height choice
 */
bool attacks_can_hit(SwordHeight attacker_height, SwordHeight defender_height) {
	// HIGH beats LOW
	if (attacker_height == SWORD_HEIGHT_HIGH && defender_height == SWORD_HEIGHT_LOW)
		return true;
	
	// LOW beats HIGH
	if (attacker_height == SWORD_HEIGHT_LOW && defender_height == SWORD_HEIGHT_HIGH)
		return true;
	
	// MID blocks nothing (loses to both extremes)
	if (defender_height == SWORD_HEIGHT_MID)
		return true;
	
	// Same height: defender blocks attacker
	if (attacker_height == defender_height)
		return false;
	
	// MID attacking: can hit MID and LOW (not HIGH which is opposite)
	if (attacker_height == SWORD_HEIGHT_MID)
		return defender_height != SWORD_HEIGHT_HIGH;
	
	return false;
}

/**
 * Create an attack action
 */
Action attacks_create_action(ActionType type, SwordHeight height, uint32_t frame) {
	Action action = {
		.type = type,
		.sword_height = height,
		.issued_frame = frame
	};
	return action;
}
