#include "constants.h"
#include "game.h"
#include "engine.h"
#include "engine/render.h"
#include "engine/input.h"


int main(void) {
	GameConfig config = {0};
	GameState state = {0};
	GameError error = engine_init();
	if (error != GAME_OK) {
		SDL_Log("engine_init failed: %s", game_error_string(error));
		(void)game_destroy(&state);
		return 1;
	}

	error = game_create(&config, &state); 
	if (error != GAME_OK) {
		SDL_Log("game_create failed: %s", game_error_string(error));
		return 1;
	}
	
	while (state.running) {
		FrameInput input = {0};
		engine_collect_input(&input);

		error = game_update(&state, &input);
		if (error != GAME_OK) {
			SDL_Log("game_update failed: %s", game_error_string(error));
			break;
		}

		error = render_frame(&state);
		if (error != GAME_OK) {
			SDL_Log("render_frame failed: %s", game_error_string(error));
			break;
		}

		SDL_Delay(1000U / TARGET_FPS);
	}

	(void)engine_shutdown();
	(void)game_destroy(&state);
	return error == GAME_OK ? 0 : 1;
}
