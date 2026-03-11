#include "input.h"
#include "types.h"
#include <SDL3/SDL.h>


void engine_collect_input(FrameInput *input) {
	SDL_Event event;

	if (input == NULL) return;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT)
			input->quit_requested = true;
	}

	const bool *keys = SDL_GetKeyboardState(NULL);

	/* Quit / pause / start */
	if (keys[SDL_SCANCODE_ESCAPE]) input->quit_requested = true;
	if (keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_SPACE])
		input->start_pressed = true;
	if (keys[SDL_SCANCODE_P]) input->pause_pressed = true;

	/* Player 1 — arrow keys + Z=attack, X=parry, A=high, S=low */
	if (keys[SDL_SCANCODE_LEFT])  input->commands[0].move_axis = -1;
	if (keys[SDL_SCANCODE_RIGHT]) input->commands[0].move_axis =  1;
	if (keys[SDL_SCANCODE_UP])    input->commands[0].jump = true;
	if (keys[SDL_SCANCODE_Z])     input->commands[0].attack = true;
	if (keys[SDL_SCANCODE_X])     input->commands[0].parry  = true;

	if      (keys[SDL_SCANCODE_A]) input->commands[0].target_height = SWORD_HEIGHT_HIGH;
	else if (keys[SDL_SCANCODE_S]) input->commands[0].target_height = SWORD_HEIGHT_LOW;
	else                           input->commands[0].target_height = SWORD_HEIGHT_MID;
}
