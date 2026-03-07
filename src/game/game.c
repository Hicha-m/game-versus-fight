#include "game.h"

GameError game_create(const GameConfig *config, GameState *out_state) {
	(void)config;
	if (out_state != NULL) {
		out_state->running = true;
		out_state->frame_index = 0;
	}
	return GAME_OK;
}

GameError game_update(GameState *state, const FrameInput *input) {
	(void)input;
	if (state != NULL) {
		state->frame_index++;
	}
	return GAME_OK;
}

GameError game_destroy(GameState *state) {
	if (state != NULL) {
		state->running = false;
	}
	return GAME_OK;
}

const char *game_error_string(GameError error) {
	(void)error;
	return "mock";
}
