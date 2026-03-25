#include <SDL3/SDL.h>

#include "core/constants.h"
#include "render/render.h"
#include "render_internal.h"
#include "utils/math_utils.h"

bool render_init(RenderContext* render, Engine* engine)
{
    (void)engine;

    if (!render) {
        return false;
    }

    render->debug_draw = false;
    render->camera.position.x = 0.0f;
    render->camera.position.y = 0.0f;
    render->camera.zoom = 1.0f;
    return true;
}

void render_shutdown(RenderContext* render)
{
    (void)render;
}

void render_update_camera(RenderContext* render, const Game* game)
{
    const Room* room;
    f32 room_width_px;
    f32 center_x;
    f32 max_camera_x;

    if (!render || !game) {
        return;
    }

    room = arena_get_current_room_const(&game->arena);
    if (!room) {
        return;
    }

    room_width_px = (f32)(room->width_tiles * TILE_SIZE);

    center_x = (game->combat.fighters[0].state.pos.x + game->combat.fighters[1].state.pos.x) * 0.5f;
    center_x += PLAYER_WIDTH * 0.5f;

    max_camera_x = room_width_px - (f32)WINDOW_WIDTH;
    math_clamp_min_zero_f32(&max_camera_x);

    render->camera.position.x = math_clampf(center_x - (f32)WINDOW_WIDTH * 0.5f, 0.0f, max_camera_x);
    render->camera.position.y = 0.0f;
}

void render_draw_room(RenderContext* render, Engine* engine, const Room* room)
{
    i32 y;
    i32 x;

    if (!render || !engine || !room) {
        return;
    }

    for (y = 0; y < room->height_tiles; ++y) {
        for (x = 0; x < room->width_tiles; ++x) {
            TileType tile = room->tiles[y][x];
            SDL_FRect r;

            if (tile == TILE_EMPTY) {
                continue;
            }

            r.x = (f32)(x * TILE_SIZE) - render->camera.position.x;
            r.y = (f32)(y * TILE_SIZE) - render->camera.position.y;
            r.w = (f32)TILE_SIZE;
            r.h = (f32)TILE_SIZE;

            if (tile == TILE_SOLID) {
                SDL_SetRenderDrawColor(engine->renderer, 62, 62, 70, 255);
            } else if (tile == TILE_PLATFORM) {
                SDL_SetRenderDrawColor(engine->renderer, 95, 95, 110, 255);
            } else {
                SDL_SetRenderDrawColor(engine->renderer, 150, 40, 40, 255);
            }

            SDL_RenderFillRect(engine->renderer, &r);
        }
    }
}

static void draw_sword_line_indicator(
    RenderContext* render,
    Engine* engine,
    const Fighter* fighter,
    u8 r,
    u8 g,
    u8 b
)
{
    SDL_FRect line_rect;
    f32 x = fighter->state.pos.x - render->camera.position.x;
    f32 y = fighter->state.pos.y - render->camera.position.y;

    line_rect.w = 14.0f;
    line_rect.h = 5.0f;

    if (fighter->state.sword_line == SWORD_LINE_HIGH) {
        line_rect.y = y + 8.0f;
    } else if (fighter->state.sword_line == SWORD_LINE_LOW) {
        line_rect.y = y + PLAYER_HEIGHT - 12.0f;
    } else {
        line_rect.y = y + PLAYER_HEIGHT * 0.5f - 2.0f;
    }

    if (fighter->state.facing_right) {
        line_rect.x = x + PLAYER_WIDTH;
    } else {
        line_rect.x = x - line_rect.w;
    }

    SDL_SetRenderDrawColor(engine->renderer, r, g, b, 255);
    SDL_RenderFillRect(engine->renderer, &line_rect);
}

void render_draw_fighters(RenderContext* render, Engine* engine, const Game* game)
{
    const Fighter* p1 = &game->combat.fighters[0];
    const Fighter* p2 = &game->combat.fighters[1];

    if (p1->state.alive) {
        SDL_FRect r1;
        r1.x = p1->state.pos.x - render->camera.position.x;
        r1.y = p1->state.pos.y - render->camera.position.y;
        r1.w = PLAYER_WIDTH;
        r1.h = PLAYER_HEIGHT;

        SDL_SetRenderDrawColor(engine->renderer, 255, 176, 43, 255);
        SDL_RenderFillRect(engine->renderer, &r1);
        draw_sword_line_indicator(render, engine, p1, 255, 235, 190);
    }

    if (p2->state.alive) {
        SDL_FRect r2;
        r2.x = p2->state.pos.x - render->camera.position.x;
        r2.y = p2->state.pos.y - render->camera.position.y;
        r2.w = PLAYER_WIDTH;
        r2.h = PLAYER_HEIGHT;

        SDL_SetRenderDrawColor(engine->renderer, 72, 170, 255, 255);
        SDL_RenderFillRect(engine->renderer, &r2);
        draw_sword_line_indicator(render, engine, p2, 180, 220, 255);
    }
}

void render_frame(RenderContext* render, Engine* engine, const Game* game, f32 alpha)
{
    const Room* room;

    (void)alpha;

    if (!render || !engine || !game || !engine->renderer) {
        return;
    }

    room = arena_get_current_room_const(&game->arena);
    if (!room) {
        return;
    }

    render_update_camera(render, game);

    SDL_SetRenderDrawColor(engine->renderer, 24, 24, 31, 255);
    SDL_RenderClear(engine->renderer);

    render_draw_room(render, engine, room);
    render_draw_fighters(render, engine, game);

    SDL_RenderPresent(engine->renderer);
}
