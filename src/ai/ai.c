#include "ai.h"

GameError ai_choose_action(const GameState *state, PlayerId actor, uint32_t budget_us, Action *out_action) {
	(void)state;
	(void)actor;
	(void)budget_us;
	if (out_action != NULL) {
		out_action->type = ACTION_NONE;
		out_action->sword_height = SWORD_HEIGHT_MID;
		out_action->issued_frame = 0;
	}
	return GAME_OK;
}

int32_t ai_evaluate_state(const GameState *state, PlayerId perspective) {
	(void)state;
	(void)perspective;
	return 0;
}

GameError ai_set_difficulty(GameState *state, DifficultyLevel difficulty) {
	if (state != NULL) {
		state->ai_difficulty = difficulty;
	}
	return GAME_OK;
}

uint8_t ai_search_depth_for_difficulty(DifficultyLevel difficulty) {
	(void)difficulty;
	return 1;
}
