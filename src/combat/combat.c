#include "combat.h"
#include "combat/character.h"
#include "combat/physics.h"
#include "combat/attacks.h"
#include "combat/momentum.h"
#include "combat/sword.h"
#include "constants.h"

/**
 * Initialize combat state for a new round
 * 
 * Sets up fighters with initial positions from arena spawn points
 */
GameError combat_init(CombatState *combat, const Arena *arena, const GameConfig *config) {
	if (combat == NULL || arena == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	// Find spawn points in arena
	Vec2f spawn_p1 = {.x = 2.0f, .y = 2.0f};
	Vec2f spawn_p2 = {.x = (float)(arena->width - 3), .y = 2.0f};
	
	// Initialize fighters
	character_init(&combat->fighters[PLAYER_ONE], spawn_p1.x, spawn_p1.y);
	character_init(&combat->fighters[PLAYER_TWO], spawn_p2.x, spawn_p2.y);
	
	combat->fighters[PLAYER_ONE].facing = FACING_RIGHT;
	combat->fighters[PLAYER_TWO].facing = FACING_LEFT;
	
	combat->round_time_frames = 0;
	combat->score[PLAYER_ONE] = 0;
	combat->score[PLAYER_TWO] = 0;
	combat->duel_active = true;
	
	return GAME_OK;
}

/**
 * Apply an action (attack, parry, etc.) from a player
 */
GameError combat_apply_action(CombatState *combat, PlayerId actor, const Action *action) {
	if (combat == NULL || action == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	// Actions are queued and processed during combat_step
	// This function validates the action is legal for the current state
	
	if (!combat->fighters[actor].alive) {
		return GAME_ERROR_INVALID_STATE;
	}
	
	// Action types are handled in combat_step with timing checks
	// (startup, active, recovery frames)
	
	return GAME_OK;
}

/**
 * Update combat state for one fixed frame (16.67ms)
 * 
 * Processes:
 * 1. Physics resolution (gravity, collision)
 * 2. Character movement from input
 * 3. Attack/parry hit detection and resolution
 * 4. Momentum frame increment / decay
 * 5. Round end conditions check
 */
GameError combat_step(CombatState *combat, const Arena *arena, uint32_t fixed_dt_ms) {
	if (combat == NULL || arena == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	if (!combat->duel_active) {
		return GAME_OK;
	}
	
	// Update each fighter
	for (int i = 0; i < 2; i++) {
		FighterState *fighter = &combat->fighters[i];
		
		if (!fighter->alive) continue;
		
		// 1. Apply physics (gravity, collision)
		physics_resolve_collision(arena, fighter);
		physics_clamp_to_arena(arena, fighter);
		
		// 2. Apply momentum speed bonus to velocity if active
		if (fighter->momentum_frames > 0) {
			float speed_multiplier = momentum_get_speed_bonus(fighter->momentum_frames);
			fighter->velocity.x *= speed_multiplier;
		}
		
		// 3. Increment momentum counter (rewards living longer)
		fighter->momentum_frames++;
		
		// 4. Check reach bonus from successful parries
		float reach_multiplier = sword_get_reach_bonus(fighter->successful_parries);
		(void)reach_multiplier;  // TODO: Apply to hitbox when collision checking
	}
	
	// Increment round timer
	combat->round_time_frames++;
	
	return GAME_OK;
}

/**
 * Reset combat for a new round
 */
GameError combat_reset_round(CombatState *combat, const Arena *arena, const GameConfig *config) {
	if (combat == NULL || arena == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	return combat_init(combat, arena, config);
}

/**
 * Check if round is over (reached goal or timeout)
 * 
 * Returns true if round is over
 * out_winner: which player won (or PLAYER_ONE if timeout)
 */
bool combat_is_round_over(const CombatState *combat, PlayerId *out_winner) {
	if (combat == NULL) return false;
	if (!combat->duel_active) return false;
	
	// Check timeout
	uint32_t max_frames = (MAX_ROUND_TIME_SECONDS * 1000) / FIXED_TIMESTEP_MS;
	if (combat->round_time_frames >= max_frames) {
		if (out_winner != NULL) {
			*out_winner = PLAYER_ONE;  // Default tie winner
		}
		return true;
	}
	
	// Check if either fighter reached goal (end of arena)
	// Low X = Player 1 goal, High X = Player 2 goal
	if (combat->fighters[PLAYER_ONE].position.x <= 0.0f) {
		if (out_winner != NULL) *out_winner = PLAYER_ONE;
		return true;
	}
	
	if (combat->fighters[PLAYER_TWO].position.x >= (float)combat->round_time_frames) {  // TODO: get arena width
		if (out_winner != NULL) *out_winner = PLAYER_TWO;
		return true;
	}
	
	return false;
}
