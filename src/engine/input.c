#include <string.h>

#include "engine/input.h"
#include "engine_internal.h"

void update_button(InputState* out, bool current_down, bool previous_down)
{
    InputState b;
    b.down = current_down;
    b.pressed = (current_down && !previous_down);
    b.released = (!current_down && previous_down);

    if (out) {
        *out = b;
    }
}

void fill_player_input_from_keybind(
    const bool* keys,
    const PlayerKeybind* bind,
    const PlayerInput* previous,
    PlayerInput* out_current
)
{
    update_button(&out_current->actions[ACTION_RIGHT],
                  keys[bind->right],
                  previous->actions[ACTION_RIGHT].down);

    update_button(&out_current->actions[ACTION_LEFT],
                  keys[bind->left],
                  previous->actions[ACTION_LEFT].down);

    update_button(&out_current->actions[ACTION_UP],
                  keys[bind->up],
                  previous->actions[ACTION_UP].down);

    update_button(&out_current->actions[ACTION_DOWN],
                  keys[bind->down],
                  previous->actions[ACTION_DOWN].down);

    update_button(&out_current->actions[ACTION_JUMP],
                  keys[bind->jump],
                  previous->actions[ACTION_JUMP].down);

    update_button(&out_current->actions[ACTION_THRUST],
                  keys[bind->thrust],
                  previous->actions[ACTION_THRUST].down);

    update_button(&out_current->actions[ACTION_THROW],
                  keys[bind->throw_weapon],
                  previous->actions[ACTION_THROW].down);
}

void input_clear_edges(FrameInput* input)
{
    if (!input) {
        return;
    }

    for (int p = 0; p < MAX_PLAYERS; p++) {
        for (int a = 0; a < ACTION_COUNT; a++) {
            input->players[p].actions[a].pressed = false;
            input->players[p].actions[a].released = false;
        }
    }

    input->pause.pressed = false;
    input->pause.released = false;
}

void input_accumulate_frame(FrameInput* accumulated, const FrameInput* sampled)
{
    if (!accumulated || !sampled) {
        return;
    }

    for (int p = 0; p < MAX_PLAYERS; p++) {
        for (int a = 0; a < ACTION_COUNT; a++) {
            accumulated->players[p].actions[a].down = sampled->players[p].actions[a].down;
            accumulated->players[p].actions[a].pressed =
                accumulated->players[p].actions[a].pressed || sampled->players[p].actions[a].pressed;
            accumulated->players[p].actions[a].released =
                accumulated->players[p].actions[a].released || sampled->players[p].actions[a].released;
        }
    }

    accumulated->pause.down = sampled->pause.down;
    accumulated->pause.pressed = accumulated->pause.pressed || sampled->pause.pressed;
    accumulated->pause.released = accumulated->pause.released || sampled->pause.released;
    accumulated->quit_requested = accumulated->quit_requested || sampled->quit_requested;
}

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
    keybinds[0].throw_weapon = SDL_SCANCODE_K;

    keybinds[1].right = SDL_SCANCODE_RIGHT;
    keybinds[1].left = SDL_SCANCODE_LEFT;
    keybinds[1].down = SDL_SCANCODE_DOWN;
    keybinds[1].up = SDL_SCANCODE_UP;
    keybinds[1].jump = SDL_SCANCODE_RSHIFT;
    keybinds[1].thrust = SDL_SCANCODE_M;
    keybinds[1].throw_weapon = SDL_SCANCODE_KP_1;
}
