#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#include "engine/engine.h"
#include "game/game.h"

typedef struct Camera {
    Vec2 position;
    f32 zoom;
} Camera;

typedef struct RenderContext {
    bool debug_draw;
    Camera camera;
} RenderContext;

/* Lifecycle */
bool render_init(RenderContext* render, Engine* engine);
void render_shutdown(RenderContext* render);

/* Frame */
void render_frame(RenderContext* render, Engine* engine, const Game* game, f32 alpha);

#endif


