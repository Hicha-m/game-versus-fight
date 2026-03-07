#include "engine.h"

GameError engine_init(GameState *state, const GameConfig *config) {
	if (state != NULL) {
		state->phase = MATCH_PHASE_MENU;
		state->frame_index = 0;
		state->running = true;
		if (config != NULL) {
			state->ai_difficulty = config->ai_difficulty;
		}
	}
	return GAME_OK;
}

GameError engine_apply_frame_input(GameState *state, const FrameInput *input) {
	(void)input;
	if (state != NULL) {
		state->running = true;
	}
	return GAME_OK;
}

GameError engine_simulate_frame(GameState *state) {
	if (state != NULL) {
		state->frame_index++;
	}
	return GAME_OK;
}

GameError engine_tick(GameState *state, const FrameInput *input) {
	(void)input;
	if (state != NULL) {
		state->frame_index++;
	}
	return GAME_OK;
}

GameError engine_shutdown(GameState *state) {
	if (state != NULL) {
		state->running = false;
	}
	return GAME_OK;
}
