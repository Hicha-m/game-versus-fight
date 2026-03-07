#include "constants.h"
#include "game.h"
#include "ui.h"

#include <SDL3/SDL.h>

static void collect_frame_input(FrameInput *input) {
	SDL_Event event;

	if (input == NULL) {
		return;
	}

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			input->quit_requested = true;
		}
	}

	const bool *keyboard = SDL_GetKeyboardState(NULL);
	input->quit_requested = input->quit_requested || keyboard[SDL_SCANCODE_ESCAPE];
}

int main(void) {
	GameConfig config = {0};
	GameState state = {0};
	GameError error = game_create(&config, &state);

	if (error != GAME_OK) {
		SDL_Log("game_create failed: %s", game_error_string(error));
		return 1;
	}

	error = ui_init();
	if (error != GAME_OK) {
		SDL_Log("ui_init failed: %s", game_error_string(error));
		(void)game_destroy(&state);
		return 1;
	}

	while (state.running) {
		FrameInput input = {0};
		collect_frame_input(&input);

		error = game_update(&state, &input);
		if (error != GAME_OK) {
			SDL_Log("game_update failed: %s", game_error_string(error));
			break;
		}

		error = ui_render_menu(&state);
		if (error != GAME_OK) {
			SDL_Log("ui_render_menu failed: %s", game_error_string(error));
			break;
		}

		SDL_Delay(1000U / TARGET_FPS);
	}

	(void)ui_shutdown();
	(void)game_destroy(&state);
	return error == GAME_OK ? 0 : 1;
}
