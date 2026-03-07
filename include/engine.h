#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"

GameError engine_init(GameState *state, const GameConfig *config);
GameError engine_apply_frame_input(GameState *state, const FrameInput *input);
GameError engine_simulate_frame(GameState *state);
GameError engine_tick(GameState *state, const FrameInput *input);
GameError engine_shutdown(GameState *state);

#endif
