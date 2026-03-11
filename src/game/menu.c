#include "menu.h"
#include "engine.h"

#include <SDL3/SDL.h>


GameError menu_render(const GameState *state) {
	if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	SDL_Renderer *renderer = engine_renderer_get();
	if (renderer == NULL) return GAME_ERROR_INVALID_STATE;

	/* Background gradient (dark navy) */
	SDL_SetRenderDrawColor(renderer, 15, 20, 45, 255);
	SDL_FRect bg = {0, 0, 1280, 720};
	SDL_RenderFillRect(renderer, &bg);

	/* Title banner */
	SDL_SetRenderDrawColor(renderer, 200, 160, 30, 255);
	SDL_FRect banner = {200, 180, 880, 10};
	SDL_RenderFillRect(renderer, &banner);
	SDL_FRect banner2 = {200, 280, 880, 10};
	SDL_RenderFillRect(renderer, &banner2);

	/* Title text at 4x scale */
	SDL_SetRenderScale(renderer, 4.0f, 4.0f);
	SDL_SetRenderDrawColor(renderer, 255, 220, 50, 255);
	SDL_RenderDebugText(renderer, 116.0f, 50.0f, "BLADE RUSH");
	SDL_SetRenderScale(renderer, 1.0f, 1.0f);

	/* Instruction / state-specific message at 2x scale */
	SDL_SetRenderScale(renderer, 2.0f, 2.0f);
	SDL_SetRenderDrawColor(renderer, 200, 200, 255, 255);

	if (state->game_phase == GAME_PHASE_GAME_OVER) {
		/* Show winner */
		bool p1_won = state->combat.score[PLAYER_ONE] >=
		             state->combat.score[PLAYER_TWO];
		SDL_RenderDebugText(renderer, 220.0f, 155.0f, "GAME OVER");
		if (p1_won)
			SDL_RenderDebugText(renderer, 195.0f, 185.0f, "PLAYER 1 WINS!");
		else
			SDL_RenderDebugText(renderer, 210.0f, 185.0f, "CPU WINS!");
		SDL_SetRenderDrawColor(renderer, 150, 150, 200, 255);
		SDL_RenderDebugText(renderer, 175.0f, 225.0f, "PRESS ENTER TO PLAY AGAIN");
	} else {
		SDL_RenderDebugText(renderer, 215.0f, 175.0f, "PRESS ENTER TO START");
		/* Controls hint */
		SDL_SetRenderDrawColor(renderer, 100, 120, 180, 255);
		SDL_RenderDebugText(renderer, 165.0f, 215.0f, "ARROWS:MOVE  UP:JUMP  Z:ATTACK  X:PARRY");
		SDL_RenderDebugText(renderer, 215.0f, 235.0f, "A:HIGH  S:LOW  (DEFAULT:MID)");
	}

	SDL_SetRenderScale(renderer, 1.0f, 1.0f);
	return GAME_OK;
}
