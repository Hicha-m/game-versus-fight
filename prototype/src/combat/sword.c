#include "combat/sword.h"
#include "constants.h"
#include <math.h>

/**
 * Calculate sword reach bonus from evolution level
 * 
 * Formula: each 3 parries = 1 evolution level
 * Reach multiplier = 1.1 ^ evolution_level
 * 
 * Example:
 *   0-2 parries: 1.1^0 = 1.0x
 *   3-5 parries: 1.1^1 = 1.1x
 *   6-8 parries: 1.1^2 = 1.21x
 *   ...
 */
float sword_get_reach_bonus(uint16_t successful_parries) {
	int evolution_level = successful_parries / PARRIES_FOR_SWORD_EVOLUTION;
	
	// Compute 1.1 ^ evolution_level
	float bonus = 1.0f;
	for (int i = 0; i < evolution_level; i++) {
		bonus *= SWORD_REACH_BONUS;
	}
	
	return bonus;
}
