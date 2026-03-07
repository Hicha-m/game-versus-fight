#include "ui.h"

#include <SDL3/SDL.h>

SDL_Renderer *ui_renderer_get(void);

GameError ui_render_menu(const GameState *state) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	SDL_Renderer *renderer = ui_renderer_get();
	if (renderer == NULL) {
		return GAME_ERROR_INVALID_STATE;
	}

	SDL_SetRenderDrawColor(renderer, 20, 24, 32, 255);
	SDL_RenderClear(renderer);

	SDL_RenderPresent(renderer);
	return GAME_OK;
}
