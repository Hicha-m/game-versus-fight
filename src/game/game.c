#include "game.h"

GameError game_create(const GameConfig *config, GameState *out_state) {
	if (out_state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	*out_state = (GameState){0};
	out_state->running = true;
	out_state->frame_index = 0;
	out_state->game_phase = GAME_PHASE_BOOT;

	if (config != NULL) {
		out_state->ai_difficulty = config->ai_difficulty;
	}

	return GAME_OK;
}

GameError game_update(GameState *state, const FrameInput *input) {
    if (state == NULL || input == NULL)
        return GAME_ERROR_INVALID_ARGUMENT;

    state->frame_index++;

    switch (state->game_phase) {

        case GAME_PHASE_BOOT:
            // Transition immédiate vers l’écran PRESS START
            state->game_phase = GAME_PHASE_PRESS_START;
            break;

        case GAME_PHASE_PRESS_START:
            if (false/*input->start_pressed*/)
                state->game_phase = GAME_PHASE_MAIN_MENU;
            break;

        case GAME_PHASE_MAIN_MENU:
            // TODO : navigation menu
            break;

        case GAME_PHASE_MATCH:
            // Logique du match
            //match_update(state, input);
            break;

        case GAME_PHASE_GAME_OVER:
            // TODO : logique game over
            break;

        default:
            break;
    }

    if (input->quit_requested)
        state->running = false;

    return GAME_OK;
}

GameError game_destroy(GameState *state) {
	if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	state->running = false;
	return GAME_OK;
}