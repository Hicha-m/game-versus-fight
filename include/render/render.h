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
    Vec2 camera_target;
    f32 camera_lerp_speed;
} RenderContext;

bool render_init(RenderContext* render, Engine* engine);
void render_shutdown(RenderContext* render);

void render_frame(RenderContext* render, Engine* engine, const Game* game);

#endif
