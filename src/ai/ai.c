#include "ai.h"

#include "difficulty.h"
#include "evaluator.h"
#include "minmax.h"

static Action ai_default_action(void) {
	Action action = {0};
	action.type = ACTION_NONE;
	action.sword_height = SWORD_HEIGHT_MID;
	action.issued_frame = 0U;
	return action;
}

static uint32_t ai_mix_seed(const GameState *state, PlayerId actor, DifficultyLevel difficulty) {
	uint32_t seed = 0U;
	if (state != NULL) {
		seed ^= state->rng_state;
		seed ^= (uint32_t)state->frame_index;
	}
	seed ^= ((uint32_t)actor + 1U) * 2654435761U;
	seed ^= ((uint32_t)difficulty + 7U) * 2246822519U;
	seed ^= seed >> 16;
	seed *= 2246822519U;
	seed ^= seed >> 13;
	return seed;
}

static GameError ai_pick_action(const GameState *state, PlayerId actor, DifficultyLevel difficulty,
		uint32_t budget_us, Action *out_action) {
	AiDifficultyConfig config = {0};
	AiSearchResult search = {0};
	AiActionList actions = {0};
	GameError err = GAME_OK;

	if (out_action == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	*out_action = ai_default_action();
	if (state == NULL || (actor != PLAYER_ONE && actor != PLAYER_TWO)) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	err = ai_difficulty_get_config(difficulty, &config);
	if (err != GAME_OK) {
		return err;
	}

	if (budget_us == 0U) {
		ai_evaluator_pick_fallback(state, actor, out_action);
		return GAME_ERROR_TIMEOUT;
	}

	err = ai_minmax_search(state, actor, actor, config.search_depth, budget_us, &search);
	if (err == GAME_OK) {
		*out_action = search.action;
	} else {
		ai_evaluator_pick_fallback(state, actor, out_action);
	}

	if (config.randomness_percent > 0U && ai_evaluator_generate_candidates(state, actor, &actions) == GAME_OK && actions.count > 1U) {
		const uint32_t seed = ai_mix_seed(state, actor, difficulty);
		const uint32_t roll = seed % 100U;
		if (roll < config.randomness_percent) {
			const uint8_t choice_window = (uint8_t)(1U + ((config.randomness_percent + 9U) / 10U));
			const uint8_t max_choice = choice_window < actions.count ? choice_window : actions.count;
			if (max_choice > 1U) {
				const uint8_t alt_index = (uint8_t)(1U + (seed % (uint32_t)(max_choice - 1U)));
				*out_action = actions.items[alt_index].action;
			}
		}
	}

	out_action->issued_frame = (uint32_t)state->frame_index;
	return err;
}

Action ai_get_action(const GameState *state, DifficultyLevel difficulty) {
	Action action = ai_default_action();
	AiDifficultyConfig config = {0};

	if (ai_difficulty_get_config(difficulty, &config) != GAME_OK) {
		config.decision_budget_us = 1000U;
	}
	(void)ai_pick_action(state, PLAYER_TWO, difficulty, config.decision_budget_us, &action);
	return action;
}

GameError ai_choose_action(const GameState *state, PlayerId actor, uint32_t budget_us, Action *out_action) {
	DifficultyLevel difficulty = DIFFICULTY_NORMAL;
	if (state != NULL) {
		difficulty = state->ai_difficulty;
	}
	return ai_pick_action(state, actor, difficulty, budget_us, out_action);
}

int32_t ai_evaluate_state(const GameState *state, PlayerId perspective) {
	int32_t score = 0;
	if (ai_evaluator_score_state(state, perspective, &score) != GAME_OK) {
		return 0;
	}
	return score;
}

GameError ai_set_difficulty(GameState *state, DifficultyLevel difficulty) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}
	state->ai_difficulty = ai_difficulty_sanitize(difficulty);
	return GAME_OK;
}

uint8_t ai_search_depth_for_difficulty(DifficultyLevel difficulty) {
	AiDifficultyConfig config = {0};
	if (ai_difficulty_get_config(difficulty, &config) != GAME_OK) {
		return 2U;
	}
	return config.search_depth;
}
