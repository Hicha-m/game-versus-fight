#ifndef RENDER_INTERNAL_H
#define RENDER_INTERNAL_H

#include "render/render.h"

void render_update_camera(RenderContext* render, const Game* game);
void render_draw_room(RenderContext* render, Engine* engine, const Room* room);
void render_draw_fighters(RenderContext* render, Engine* engine, const Game* game);

#endif 