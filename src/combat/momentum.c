#include "combat/momentum.h"
#include "constants.h"

/**
 * Calculate speed bonus from momentum
 * 
 * Formula: bonus = momentum_frames / DECAY_FRAMES * (MAX_BONUS - 1.0)
 * At max momentum (60 frames), speed multiplier reaches 1.5x
 * 
 * Returns: multiplier to apply to velocity (1.0 = no bonus, 1.5 = max +50%)
 */
float momentum_get_speed_bonus(uint32_t momentum_frames) {
	if (momentum_frames == 0) return 1.0f;
	
	// Linear progression from 0 to MAX
	float decay_ratio = (float)momentum_frames / (float)MOMENTUM_DECAY_FRAMES;
	if (decay_ratio > 1.0f) decay_ratio = 1.0f;
	
	// Bonus ranges from 1.0 (none) to 1.5 (max)
	float bonus = 1.0f + (MOMENTUM_SPEED_BONUS_MAX - 1.0f) * decay_ratio;
	return bonus;
}
