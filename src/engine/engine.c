#include "engine.h"
#include <SDL3/SDL.h>

static SDL_Window *s_window = NULL;
static SDL_Renderer *s_renderer = NULL;

SDL_Renderer *engine_renderer_get(void) {
	return s_renderer;
}

GameError engine_init() {
	
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("SDL_Init failed: %s", SDL_GetError());
		return GAME_ERROR_INTERNAL;
	}
	
	s_window = SDL_CreateWindow("Blade Rush", 1280, 720, 0);
	if (s_window == NULL) {
		SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
		SDL_Quit();
		return GAME_ERROR_INTERNAL;
	}

	s_renderer = SDL_CreateRenderer(s_window, NULL);
	if (s_renderer == NULL) {
		SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
		SDL_DestroyWindow(s_window);
		s_window = NULL;
		SDL_Quit();
		return GAME_ERROR_INTERNAL;
	}
	return GAME_OK;
}

GameError engine_shutdown() {

	if (s_renderer != NULL) {
		SDL_DestroyRenderer(s_renderer);
		s_renderer = NULL;
	}
	if (s_window != NULL) {
		SDL_DestroyWindow(s_window);
		s_window = NULL;
	}
	SDL_Quit();
		
	
	return GAME_OK;
}
