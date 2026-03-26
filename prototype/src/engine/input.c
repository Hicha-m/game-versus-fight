#include "input.h"

#include <SDL3/SDL.h>
#include <string.h>

static bool key_down(const bool *keyboard, int32_t scancode) {
	if (keyboard == NULL || scancode < 0 || scancode >= SDL_SCANCODE_COUNT) {
		return false;
	}

	return keyboard[scancode];
}

static int32_t binding_for(const GameState *state, PlayerId player, BindingAction action, SDL_Scancode fallback) {
	if (state == NULL) {
		return fallback;
	}

	return state->config.bindings[player].scancodes[action] > 0
		? state->config.bindings[player].scancodes[action]
		: (int32_t)fallback;
}

static void populate_player_command(FrameInput *input, const GameState *state, const bool *keyboard, PlayerId player) {
	const int32_t left = binding_for(state, player, BIND_MOVE_LEFT, player == PLAYER_ONE ? SDL_SCANCODE_LEFT : SDL_SCANCODE_A);
	const int32_t right = binding_for(state, player, BIND_MOVE_RIGHT, player == PLAYER_ONE ? SDL_SCANCODE_RIGHT : SDL_SCANCODE_D);
	const int32_t jump = binding_for(state, player, BIND_JUMP, player == PLAYER_ONE ? SDL_SCANCODE_RSHIFT : SDL_SCANCODE_W);
	const int32_t thrust = binding_for(state, player, BIND_ATTACK, player == PLAYER_ONE ? SDL_SCANCODE_RETURN : SDL_SCANCODE_F);
	const int32_t sword_throw = binding_for(state, player, BIND_PARRY, player == PLAYER_ONE ? SDL_SCANCODE_RCTRL : SDL_SCANCODE_G);
	const int32_t stance_up = binding_for(state, player, BIND_STANCE_UP, player == PLAYER_ONE ? SDL_SCANCODE_UP : SDL_SCANCODE_Q);
	const int32_t stance_down = binding_for(state, player, BIND_STANCE_DOWN, player == PLAYER_ONE ? SDL_SCANCODE_DOWN : SDL_SCANCODE_S);

	PlayerCommand *command = &input->commands[player];
	command->target_height = SWORD_HEIGHT_MID;
	if (key_down(keyboard, left)) {
		command->move_axis = -1;
	}
	if (key_down(keyboard, right)) {
		command->move_axis = 1;
	}
	command->jump = key_down(keyboard, jump);
	command->attack = key_down(keyboard, thrust);
	command->parry = key_down(keyboard, sword_throw);
	command->crouch = key_down(keyboard, stance_down);
	if (key_down(keyboard, stance_up)) {
		command->target_height = SWORD_HEIGHT_HIGH;
	} else if (key_down(keyboard, stance_down)) {
		command->target_height = SWORD_HEIGHT_LOW;
	}
}

void engine_collect_input(FrameInput *input, const GameState *state) {
	if (input == NULL) {
		return;
	}

	memset(input, 0, sizeof(*input));
	input->pressed_scancode = -1;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			input->quit_requested = true;
		}
		if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
			input->any_key_pressed = true;
			input->pressed_scancode = event.key.scancode;
			switch (event.key.scancode) {
			case SDL_SCANCODE_UP: input->menu_up_pressed = true; break;
			case SDL_SCANCODE_DOWN: input->menu_down_pressed = true; break;
			case SDL_SCANCODE_LEFT: input->menu_left_pressed = true; break;
			case SDL_SCANCODE_RIGHT: input->menu_right_pressed = true; break;
			case SDL_SCANCODE_RETURN:
			case SDL_SCANCODE_SPACE:
				input->menu_confirm_pressed = true;
				break;
			case SDL_SCANCODE_ESCAPE:
				input->menu_back_pressed = true;
				break;
			default:
				break;
			}
		}
	}

	const bool *keyboard = SDL_GetKeyboardState(NULL);
	populate_player_command(input, state, keyboard, PLAYER_ONE);
	populate_player_command(input, state, keyboard, PLAYER_TWO);
}
