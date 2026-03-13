#ifndef GAME_MATCH_H
#define GAME_MATCH_H

#include "types.h"

GameError match_start(GameState *state);
GameError match_update(GameState *state, const FrameInput *input);
void match_abort(GameState *state);

#endif