#ifndef INPUT_H
#define INPUT_H

#include "core/types.h"
#include "core/constants.h"

typedef struct InputButton {
    bool down;
    bool pressed;
    bool released;
} InputButton;


typedef struct PlayerInput {
    i32 move_x; /* -1, 0, +1 */
    i32 move_y; /* +1 = haut, 0 = neutre, -1 = bas */

    InputButton jump;
    InputButton thrust;
    InputButton throw_weapon;
} PlayerInput;


typedef struct FrameInput {
    PlayerInput players[MAX_PLAYERS];
    InputButton pause;
    bool quit_requested;
} FrameInput;


/* Helpers */
void input_reset_frame(FrameInput* input);

#endif