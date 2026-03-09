#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

#include "types.h"

void render_begin();
void render_end();
GameError render_frame(const GameState *state);
GameError render_world(const GameState *state);
#endif