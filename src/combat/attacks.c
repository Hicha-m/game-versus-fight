#include "combat/attacks.h"

/**
 * Nidhogg-style sword lane rule:
 * - Matching sword height auto-blocks.
 * - Different heights do not block each other.
 */
bool attacks_can_hit(SwordHeight attacker_height, SwordHeight defender_height) {
	return attacker_height != defender_height;
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
