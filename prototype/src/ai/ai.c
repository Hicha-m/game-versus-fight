#include "ai.h"
#include "ai/minmax.h"
#include "ai/state.h"
#include "ai/evaluator.h"
#include "constants.h"

/**
 * Choose best AI action using MinMax search
 * 
 * Time-budgeted: uses AI_TIME_BUDGET_US microseconds max
 */
GameError ai_choose_action(const GameState *state, PlayerId actor, uint32_t budget_us, Action *out_action) {
	if (state == NULL || out_action == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	
	// Get search depth based on difficulty
	uint8_t depth = ai_search_depth_for_difficulty(state->ai_difficulty);
	
	// Run MinMax search with time budget
	MinMaxResult result = minmax_search(&state->combat, &state->arena, actor, depth, budget_us);
	
	*out_action = result.best_action;
	return GAME_OK;
}

/**
 * Evaluate the current game state from a perspective
 */
int32_t ai_evaluate_state(const GameState *state, PlayerId perspective) {
	if (state == NULL) return 0;
	
	AIState ai_state = ai_state_discretize(state);
	return evaluator_score(&ai_state, perspective, &state->arena);
}

/**
 * Set AI difficulty in game state
 */
GameError ai_set_difficulty(GameState *state, DifficultyLevel difficulty) {
	if (state != NULL) {
		state->ai_difficulty = difficulty;
	}
	return GAME_OK;
}

/**
 * Return search depth for difficulty level
 * 
 * Architecture: Depth controls quality vs performance tradeoff
 * At depth 4 with alpha-beta, searches ~1000 nodes in <1ms
 */
uint8_t ai_search_depth_for_difficulty(DifficultyLevel difficulty) {
	switch (difficulty) {
		case DIFFICULTY_EASY:   return 1;
		case DIFFICULTY_NORMAL: return 2;
		case DIFFICULTY_HARD:   return 3;
		case DIFFICULTY_EXPERT: return 4;
		default:                return 2;
	}
}
