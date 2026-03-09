#include "input.h"
#include "types.h"
#include <SDL3/SDL.h>


void engine_collect_input(FrameInput *input) {
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
