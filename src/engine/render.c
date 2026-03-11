#include "render.h"
#include "engine.h"
#include "arena.h"
#include "constants.h"
#include "game/menu.h"
#include "game/hud.h"

#include <SDL3/SDL.h>


void render_begin(void) {
    SDL_Renderer *renderer = engine_renderer_get();
    if (!renderer) return;
    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 255);
    SDL_RenderClear(renderer);
}

void render_end(void) {
    SDL_Renderer *renderer = engine_renderer_get();
    if (!renderer) return;
    SDL_RenderPresent(renderer);
}

GameError render_frame(const GameState *state) {
    if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

    render_begin();

    switch (state->game_phase) {
        case GAME_PHASE_PRESS_START:
        case GAME_PHASE_MAIN_MENU:
        case GAME_PHASE_MODE_SELECT:
        case GAME_PHASE_GAME_OVER:
            menu_render(state);
            break;

        case GAME_PHASE_MATCH:
            render_world(state);
            hud_render(state);
            break;

        default:
            break;
    }

    render_end();
    return GAME_OK;
}

static float sword_y(const FighterState *f) {
    switch (f->sword_height) {
        case SWORD_HEIGHT_HIGH: return f->position.y + 6.0f;
        case SWORD_HEIGHT_LOW:  return f->position.y + (float)FIGHTER_H - 18.0f;
        default:                return f->position.y + (float)FIGHTER_H * 0.5f - 4.0f;
    }
}

GameError render_world(const GameState *state) {
    if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

    SDL_Renderer *renderer = engine_renderer_get();
    if (!renderer) return GAME_ERROR_INVALID_STATE;

    const Arena *arena = &state->arena;

    /* Draw arena tiles */
    if (arena->tiles != NULL) {
        for (uint16_t row = 0; row < arena->height; row++) {
            for (uint16_t col = 0; col < arena->width; col++) {
                TileType t = arena_get_tile(arena, col, row, NULL);
                float rx = (float)(col * ARENA_TILE_W);
                float ry = (float)(row * ARENA_TILE_H);

                if (t == TILE_SOLID) {
                    SDL_SetRenderDrawColor(renderer, 90, 90, 110, 255);
                    SDL_FRect r = {rx, ry, (float)ARENA_TILE_W, (float)ARENA_TILE_H};
                    SDL_RenderFillRect(renderer, &r);
                    SDL_SetRenderDrawColor(renderer, 130, 130, 150, 255);
                    SDL_RenderRect(renderer, &r);
                } else if (t == TILE_PLATFORM) {
                    SDL_SetRenderDrawColor(renderer, 110, 75, 35, 255);
                    SDL_FRect r = {rx, ry, (float)ARENA_TILE_W, 12.0f};
                    SDL_RenderFillRect(renderer, &r);
                } else if (t == TILE_HAZARD) {
                    SDL_SetRenderDrawColor(renderer, 200, 40, 40, 255);
                    SDL_FRect r = {rx, ry, (float)ARENA_TILE_W, (float)ARENA_TILE_H};
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        }
    }

    /* Draw fighters */
    for (int i = 0; i < 2; i++) {
        const FighterState *f = &state->combat.fighters[i];
        if (!f->alive) continue;

        SDL_FRect body = {f->position.x, f->position.y,
                          (float)FIGHTER_W, (float)FIGHTER_H};
        if (i == PLAYER_ONE)
            SDL_SetRenderDrawColor(renderer, 60, 120, 220, 255);
        else
            SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
        SDL_RenderFillRect(renderer, &body);

        SDL_SetRenderDrawColor(renderer, 220, 220, 255, 200);
        SDL_RenderRect(renderer, &body);

        /* Sword indicator */
        float sx = (f->facing == FACING_RIGHT)
                 ? f->position.x + (float)FIGHTER_W
                 : f->position.x - 16.0f;
        SDL_FRect sword = {sx, sword_y(f), 16.0f, 5.0f};
        if (i == PLAYER_ONE)
            SDL_SetRenderDrawColor(renderer, 160, 210, 255, 255);
        else
            SDL_SetRenderDrawColor(renderer, 255, 160, 160, 255);
        SDL_RenderFillRect(renderer, &sword);
    }

    return GAME_OK;
}
