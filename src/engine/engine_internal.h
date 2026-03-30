#ifndef ENGINE_INTERNAL_H
#define ENGINE_INTERNAL_H

#include <SDL3/SDL.h>
#include "engine/engine.h"

typedef struct PlayerKeybind {
    SDL_Scancode right;
    SDL_Scancode left;
    SDL_Scancode down;
    SDL_Scancode up;

    SDL_Scancode jump;
    SDL_Scancode thrust;
    SDL_Scancode throw_weapon;
} PlayerKeybind;

struct Engine {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    PlayerKeybind keybinds[MAX_PLAYERS];
    FrameInput previous_input;
};

void update_button(InputState* out, bool current_down, bool previous_down);

void fill_player_input_from_keybind(
    const bool* keys,
    const PlayerKeybind* bind,
    const PlayerInput* previous,
    PlayerInput* out_current
);

void engine_apply_keyboard_to_frame_input(
    Engine* engine,
    const bool* keys,
    FrameInput* out_input
);

void input_set_default_keybinds(PlayerKeybind keybinds[MAX_PLAYERS]);

#endif
