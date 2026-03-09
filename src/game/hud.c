#include "hud.h"
#include "engine.h"

#include <SDL3/SDL.h>

GameError hud_render(const GameState *state) {
	if (state == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	SDL_Renderer *renderer = engine_renderer_get();
	if (renderer == NULL) return GAME_ERROR_INVALID_STATE;


	/*
	SDL_SetRenderDrawColor(renderer, 24, 36, 56, 255);
	SDL_FRect frame = {180.0f, 100.0f, 920.0f, 520.0f};
	SDL_RenderFillRect(renderer, &frame);
	TO DO
	*/


	return GAME_OK;
}