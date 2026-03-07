#include "game.h"

GameError game_create(const GameConfig *config, GameState *out_state) {
	if (out_state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	*out_state = (GameState){0};
	out_state->running = true;
	out_state->frame_index = 0;
	out_state->phase = MATCH_PHASE_MENU;

	if (config != NULL) {
		out_state->ai_difficulty = config->ai_difficulty;
	}

	return GAME_OK;
}

GameError game_update(GameState *state, const FrameInput *input) {
	if (state == NULL || input == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	state->frame_index++;

	if (input->quit_requested) {
		state->running = false;
		return GAME_OK;
	}
	return GAME_OK;
}

GameError game_destroy(GameState *state) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	state->running = false;
	return GAME_OK;
}

static const char *const g_error_strings[] = {
	"ok",                 /* GAME_OK */
	"invalid argument",  /* GAME_ERROR_INVALID_ARGUMENT */
	"invalid state",     /* GAME_ERROR_INVALID_STATE */
	"out of memory",     /* GAME_ERROR_OUT_OF_MEMORY */
	"out of bounds",     /* GAME_ERROR_OUT_OF_BOUNDS */
	"not found",         /* GAME_ERROR_NOT_FOUND */
	"unsupported",       /* GAME_ERROR_UNSUPPORTED */
	"timeout",           /* GAME_ERROR_TIMEOUT */
	"io error",          /* GAME_ERROR_IO */
	"internal error",    /* GAME_ERROR_INTERNAL */
};

const char *game_error_string(GameError error)
{
	size_t idx = (size_t)error;
	size_t count = sizeof g_error_strings / sizeof *g_error_strings;
	if (idx < count) {
		return g_error_strings[idx];
	}
	return "unknown error";
}
