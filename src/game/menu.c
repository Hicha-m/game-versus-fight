#include "menu.h"
#include "engine.h"

#include <SDL3/SDL.h>


GameError menu_render(const GameState *state) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	SDL_Renderer *renderer = engine_renderer_get();
	if (renderer == NULL) return GAME_ERROR_INVALID_STATE;

    // Exemple : fond bleu du menu
    SDL_SetRenderDrawColor(renderer, 40, 60, 120, 255);
    SDL_FRect r = { 0, 0, 1280, 720 };
    SDL_RenderFillRect(renderer, &r);
	
	return GAME_OK;
}
