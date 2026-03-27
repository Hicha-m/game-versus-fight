#include <string.h>

#include "engine/input.h"
#include "engine_internal.h"

void input_reset_frame(FrameInput* input)
{
	if (!input) {
		return;
	}

	memset(input, 0, sizeof(*input));
}

void input_set_default_keybinds(PlayerKeybind keybinds[MAX_PLAYERS])
{
	if (!keybinds) {
		return;
	}

	keybinds[0].right = SDL_SCANCODE_D;
	keybinds[0].left = SDL_SCANCODE_A;
	keybinds[0].down = SDL_SCANCODE_S;
	keybinds[0].up = SDL_SCANCODE_W;
	keybinds[0].jump = SDL_SCANCODE_SPACE;
	keybinds[0].thrust = SDL_SCANCODE_J;

	keybinds[1].right = SDL_SCANCODE_RIGHT;
	keybinds[1].left = SDL_SCANCODE_LEFT;
	keybinds[1].down = SDL_SCANCODE_DOWN;
	keybinds[1].up = SDL_SCANCODE_UP;
	keybinds[1].jump = SDL_SCANCODE_RSHIFT;
	keybinds[1].thrust = SDL_SCANCODE_KP_1;
}
