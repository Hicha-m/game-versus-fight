#ifndef RENDER_INTERNAL_H
#define RENDER_INTERNAL_H

#include <SDL3/SDL.h>
#include "render/render.h"
#include "ui/menu.h"

void render_update_camera(RenderContext* render, const Game* game);
void render_draw_room(RenderContext* render, Engine* engine, const Room* room);
void render_draw_fighters(RenderContext* render, Engine* engine, const Game* game);
void render_menu(SDL_Renderer* renderer, const MenuContext* menu);
void render_victory_screen(SDL_Renderer* renderer, const Game* game, i32 winner_index);

#endif