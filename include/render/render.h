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
    Vec2 camera_target;        /* Target position for smooth camera tracking */
    f32 camera_lerp_speed;     /* Speed of camera smooth tracking (0.15 = smooth, >1 = instant) */
} RenderContext;

/* Lifecycle */
bool render_init(RenderContext* render, Engine* engine);
void render_shutdown(RenderContext* render);

/* Frame */
void render_frame(RenderContext* render, Engine* engine, const Game* game);

#endif