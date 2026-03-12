#include "game.h"
#include "arena.h"
#include "combat.h"
#include "constants.h"
#include "utils.h"
#include <time.h>

/**
 * Initialize game state
 */
GameError game_create(const GameConfig *config, GameState *out_state) {
	if (out_state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	*out_state = (GameState){0};
	out_state->running = true;
	out_state->frame_index = 0;
	out_state->game_phase = GAME_PHASE_BOOT;
	out_state->match_phase = MATCH_PHASE_MENU;
	
	if (config != NULL) {
		out_state->ai_difficulty = config->ai_difficulty;
	} else {
		out_state->ai_difficulty = DIFFICULTY_NORMAL;
	}
	
	out_state->rng_state = (uint32_t)time(NULL);
	return GAME_OK;
}

/**
 * Initialize a new match (arena + combat)
 */
static GameError game_init_match(GameState *state, const GameConfig *config) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	uint16_t arena_width = config ? config->arena_width : 32;
	uint16_t arena_height = config ? config->arena_height : 16;
	uint32_t arena_seed = config ? config->arena_seed : state->rng_state;
	
	if (arena_width < MIN_ARENA_WIDTH) arena_width = MIN_ARENA_WIDTH;
	if (arena_width > MAX_ARENA_WIDTH) arena_width = MAX_ARENA_WIDTH;
	if (arena_height < MIN_ARENA_HEIGHT) arena_height = MIN_ARENA_HEIGHT;
	if (arena_height > MAX_ARENA_HEIGHT) arena_height = MAX_ARENA_HEIGHT;
	
	GameError err = arena_generate(&state->arena, arena_width, arena_height, arena_seed);
	if (err != GAME_OK) {
		return err;
	}
	
	err = combat_init(&state->combat, &state->arena, config);
	if (err != GAME_OK) {
		arena_destroy(&state->arena);
		return err;
	}
	
	state->match_phase = MATCH_PHASE_COUNTDOWN;
	return GAME_OK;
}

/**
 * Main game update loop
 */
GameError game_update(GameState *state, const FrameInput *input) {
	if (state == NULL || input == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	state->frame_index++;
	
	if (input->quit_requested) {
		state->running = false;
		return GAME_OK;
	}
	
	switch (state->game_phase) {
		case GAME_PHASE_BOOT:
			state->game_phase = GAME_PHASE_PRESS_START;
			break;
		
		case GAME_PHASE_PRESS_START:
			if (state->frame_index > 120) {
				state->game_phase = GAME_PHASE_MAIN_MENU;
			}
			break;
		
		case GAME_PHASE_MAIN_MENU:
			if (state->frame_index > 240) {
				GameConfig config = {
					.arena_width = 32,
					.arena_height = 16,
					.arena_seed = state->rng_state,
					.ai_difficulty = DIFFICULTY_NORMAL,
					.max_round_time_seconds = MAX_ROUND_TIME_SECONDS
				};
				GameError err = game_init_match(state, &config);
				if (err == GAME_OK) {
					state->game_phase = GAME_PHASE_MATCH;
				}
			}
			break;
		
		case GAME_PHASE_MATCH:
			if (state->match_phase == MATCH_PHASE_COUNTDOWN) {
				if (state->combat.round_time_frames > 120) {
					state->match_phase = MATCH_PHASE_FIGHT;
				}
			} else if (state->match_phase == MATCH_PHASE_FIGHT) {
				GameError err = combat_step(&state->combat, &state->arena, FIXED_TIMESTEP_MS);
				if (err != GAME_OK) {
					return err;
				}
				
				PlayerId winner;
				if (combat_is_round_over(&state->combat, &winner)) {
					state->match_phase = MATCH_PHASE_ROUND_END;
					state->combat.score[winner]++;
				}
			} else if (state->match_phase == MATCH_PHASE_ROUND_END) {
				if (state->combat.round_time_frames > 180) {
					state->game_phase = GAME_PHASE_GAME_OVER;
				}
			}
			break;
		
		case GAME_PHASE_GAME_OVER:
			if (state->frame_index > 600) {
				state->game_phase = GAME_PHASE_MAIN_MENU;
				arena_destroy(&state->arena);
				state->frame_index = 0;
			}
			break;
		
		default:
			break;
	}

	return GAME_OK;
}

/**
 * Cleanup game state
 */
GameError game_destroy(GameState *state) {
	if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;
	
	arena_destroy(&state->arena);
	state->running = false;
	return GAME_OK;
}


