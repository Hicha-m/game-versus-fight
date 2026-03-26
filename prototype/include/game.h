#ifndef GAME_H
#define GAME_H

#include "types.h"

GameError game_create(const GameConfig *config, GameState *out_state);
GameError game_update(GameState *state, const FrameInput *input);
GameError game_destroy(GameState *state);
const char *game_error_string(GameError error);

#endif
