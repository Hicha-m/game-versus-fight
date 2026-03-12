#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

#include "types.h"

void render_begin();
void render_end();

/**
 * Renders a frame with interpolation
 * interpolation_factor: [0.0, 1.0] - how far into next frame (for smooth rendering at high FPS)
 */
GameError render_frame(const GameState *state, float interpolation_factor);
GameError render_world(const GameState *state, float interpolation_factor);
#endif