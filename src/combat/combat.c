#include "combat.h"
#include "arena.h"
#include "combat/character.h"
#include "combat/physics.h"
#include "combat/attacks.h"
#include "combat/sword.h"
#include "constants.h"

static uint16_t clamp_round_time_seconds(const GameConfig *config) {
	uint16_t seconds = DEFAULT_ROUND_TIME_SECONDS;

	if (config != NULL && config->max_round_time_seconds > 0U) {
		seconds = config->max_round_time_seconds;
	}
	if (seconds < MIN_ROUND_TIME_SECONDS) {
		seconds = MIN_ROUND_TIME_SECONDS;
	}
	if (seconds > MAX_ROUND_TIME_SECONDS) {
		seconds = MAX_ROUND_TIME_SECONDS;
	}

	return seconds;
}

/**
 * Initialize combat state for a new round
 * 
 * Sets up fighters with initial positions from arena spawn points
 */
GameError combat_init(CombatState *combat, const Arena *arena, const GameConfig *config) {
	if (combat == NULL || arena == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	*combat = (CombatState){0};
	combat->arena_width = arena->width;
	combat->segment_goal_mode = (arena->width >= ARENA_DEFAULT_WIDTH);
	combat->round_time_limit_seconds = clamp_round_time_seconds(config);

	Vec2i spawn_p1 = {0};
	Vec2i spawn_p2 = {0};
	float floor_y = (float)(arena->height - 1) - PLAYER_HEIGHT;
	arena_find_spawn(arena, PLAYER_ONE, &spawn_p1);
	arena_find_spawn(arena, PLAYER_TWO, &spawn_p2);

	character_init(&combat->fighters[PLAYER_ONE], (float)spawn_p1.x, floor_y);
	character_init(&combat->fighters[PLAYER_TWO], (float)spawn_p2.x, floor_y);

	combat->fighters[PLAYER_ONE].facing = (spawn_p1.x <= spawn_p2.x) ? FACING_RIGHT : FACING_LEFT;
	combat->fighters[PLAYER_TWO].facing = (spawn_p2.x >= spawn_p1.x) ? FACING_LEFT : FACING_RIGHT;

	combat->round_time_frames = 0;
	combat->score[PLAYER_ONE] = 0;
	combat->score[PLAYER_TWO] = 0;
	combat->duel_active = true;
	combat->respawn_frames[PLAYER_ONE] = 0;
	combat->respawn_frames[PLAYER_TWO] = 0;
	combat->death_popup_frames[PLAYER_ONE] = 0;
	combat->death_popup_frames[PLAYER_TWO] = 0;
	
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
	(void)fixed_dt_ms;
	
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
		
		// 2. Increment momentum counter (rewards living longer)
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
	uint16_t round_time_limit = combat->round_time_limit_seconds;
	if (round_time_limit < MIN_ROUND_TIME_SECONDS || round_time_limit > MAX_ROUND_TIME_SECONDS) {
		round_time_limit = DEFAULT_ROUND_TIME_SECONDS;
	}
	uint32_t max_frames = ((uint32_t)round_time_limit * 1000U) / FIXED_TIMESTEP_MS;
	if (combat->round_time_frames >= max_frames) {
		if (out_winner != NULL) {
			float p1_distance = combat->fighters[PLAYER_ONE].position.x;
			float p2_distance = (float)combat->arena_width - combat->fighters[PLAYER_TWO].position.x;
			*out_winner = (p1_distance <= p2_distance) ? PLAYER_ONE : PLAYER_TWO;
		}
		return true;
	}
	
	if (combat->segment_goal_mode && combat->fighters[PLAYER_ONE].alive &&
	    combat->fighters[PLAYER_ONE].position.x < (float)MAP_SEGMENT_TILES) {
		if (out_winner != NULL) *out_winner = PLAYER_ONE;
		return true;
	}
	if (combat->segment_goal_mode && combat->fighters[PLAYER_TWO].alive &&
	    combat->arena_width > 0 &&
	    combat->fighters[PLAYER_TWO].position.x + PLAYER_WIDTH > (float)(combat->arena_width - MAP_SEGMENT_TILES)) {
		if (out_winner != NULL) *out_winner = PLAYER_TWO;
		return true;
	}
	
	return false;
}
