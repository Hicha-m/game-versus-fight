#ifndef INPUT_H
#define INPUT_H

#include "core/types.h"
#include "core/constants.h"

typedef struct InputState {
    bool down;
    bool pressed;
    bool released;
} InputState;

typedef enum {
    ACTION_RIGHT,
    ACTION_LEFT,
    ACTION_UP,
    ACTION_DOWN,
    ACTION_JUMP,
    ACTION_THRUST,
    ACTION_THROW,

    ACTION_COUNT
} ACTION;

typedef struct {
    InputState actions [ACTION_COUNT];
} PlayerInput;

typedef struct FrameInput {
    PlayerInput players[MAX_PLAYERS];
    InputState pause;
    bool quit_requested;
} FrameInput;

void input_reset_frame(FrameInput* input);
void input_clear_edges(FrameInput* input);
void input_accumulate_frame(FrameInput* accumulated, const FrameInput* sampled);

#endif
