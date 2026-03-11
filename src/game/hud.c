#include "hud.h"
#include "engine.h"

#include <SDL3/SDL.h>


GameError hud_render(const GameState *state) {
	if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	SDL_Renderer *renderer = engine_renderer_get();
	if (renderer == NULL) return GAME_ERROR_INVALID_STATE;

	const CombatState *c = &state->combat;

	/* P1 score pip dots (blue, top-left) */
	for (int i = 0; i < 3; i++) {
		SDL_FRect pip = {20.0f + (float)i * 32.0f, 12.0f, 24.0f, 24.0f};
		if (i < (int)c->score[PLAYER_ONE])
			SDL_SetRenderDrawColor(renderer, 60, 130, 230, 255);
		else
			SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
		SDL_RenderFillRect(renderer, &pip);
		SDL_SetRenderDrawColor(renderer, 100, 140, 255, 255);
		SDL_RenderRect(renderer, &pip);
	}

	/* P2 score pip dots (red, top-right) */
	for (int i = 0; i < 3; i++) {
		SDL_FRect pip = {1164.0f - (float)i * 32.0f, 12.0f, 24.0f, 24.0f};
		if (i < (int)c->score[PLAYER_TWO])
			SDL_SetRenderDrawColor(renderer, 230, 60, 60, 255);
		else
			SDL_SetRenderDrawColor(renderer, 60, 40, 40, 255);
		SDL_RenderFillRect(renderer, &pip);
		SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
		SDL_RenderRect(renderer, &pip);
	}

	/* Center labels at 2x scale */
	SDL_SetRenderScale(renderer, 2.0f, 2.0f);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDebugText(renderer, 291.0f, 8.0f, "P1");
	SDL_RenderDebugText(renderer, 567.0f, 8.0f, "VS");
	SDL_RenderDebugText(renderer, 585.0f, 8.0f, "CPU");
	SDL_SetRenderScale(renderer, 1.0f, 1.0f);

	return GAME_OK;
}
