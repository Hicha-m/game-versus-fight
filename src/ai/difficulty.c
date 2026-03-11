#include "difficulty.h"

DifficultyLevel ai_difficulty_sanitize(DifficultyLevel difficulty) {
	switch (difficulty) {
		case DIFFICULTY_EASY:
		case DIFFICULTY_NORMAL:
		case DIFFICULTY_HARD:
		case DIFFICULTY_EXPERT:
			return difficulty;
		default:
			return DIFFICULTY_NORMAL;
	}
}

GameError ai_difficulty_get_config(DifficultyLevel difficulty, AiDifficultyConfig *out_config) {
	if (out_config == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	switch (ai_difficulty_sanitize(difficulty)) {
		case DIFFICULTY_EASY:
			out_config->search_depth = 1U;
			out_config->randomness_percent = 20U;
			out_config->decision_budget_us = 250U;
			break;
		case DIFFICULTY_NORMAL:
			out_config->search_depth = 2U;
			out_config->randomness_percent = 10U;
			out_config->decision_budget_us = 500U;
			break;
		case DIFFICULTY_HARD:
			out_config->search_depth = 3U;
			out_config->randomness_percent = 5U;
			out_config->decision_budget_us = 750U;
			break;
		case DIFFICULTY_EXPERT:
			out_config->search_depth = 3U;
			out_config->randomness_percent = 0U;
			out_config->decision_budget_us = 1000U;
			break;
		default:
			return GAME_ERROR_INTERNAL;
	}

	return GAME_OK;
}
