#include "render.h"
#include "engine.h"
#include "combat/sword.h"
#include "constants.h"
#include "game/menu.h"
#include "game/hud.h"

#include <math.h>
#include <stdio.h>

static int clamp_segment_index(int value) {
    if (value < 0) return 0;
    if (value >= (int)MAP_TOTAL_SEGMENTS) return (int)MAP_TOTAL_SEGMENTS - 1;
    return value;
}

static float clamp_camera_left(float x, float max_left) {
    if (x < 0.0f) return 0.0f;
    if (x > max_left) return max_left;
    return x;
}

static float sword_lane_offset(SwordHeight height) {
    switch (height) {
    case SWORD_HEIGHT_HIGH: return 0.45f;
    case SWORD_HEIGHT_LOW: return 1.55f;
    case SWORD_HEIGHT_MID:
    default:
        return 1.0f;
    }
}

static void segment_color(int segment, Uint8 *red, Uint8 *green, Uint8 *blue) {
    static const Uint8 colors[7][3] = {
        {54, 32, 26},
        {32, 48, 74},
        {24, 64, 58},
        {42, 42, 58},
        {58, 46, 28},
        {34, 58, 74},
        {62, 26, 30},
    };
    int index = clamp_segment_index(segment);
    *red = colors[index][0];
    *green = colors[index][1];
    *blue = colors[index][2];
}

static void draw_fighter(SDL_Renderer *renderer,
                          const FighterState *fighter,
                          Uint8 red,
                          Uint8 green,
                          Uint8 blue,
                          float camera_left,
                          float view_width,
                          float tile_px,
                          float world_offset_y) {
    if (fighter == NULL || !fighter->alive) {
        return;
    }

    float px = (fighter->position.x - camera_left) * tile_px;
    float py = fighter->position.y * tile_px + world_offset_y;

    if (px + PLAYER_WIDTH * tile_px < 0.0f || px > view_width) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_FRect body = {px, py, PLAYER_WIDTH * tile_px, PLAYER_HEIGHT * tile_px};
    SDL_RenderFillRect(renderer, &body);

    if (fighter->has_sword) {
        SDL_SetRenderDrawColor(renderer, 230, 230, 180, 255);
        float sword_y = py + sword_lane_offset(fighter->sword_height) * tile_px;
        float sword_base_x = fighter->facing == FACING_RIGHT ? px + PLAYER_WIDTH * tile_px : px;
        float sword_len = BASE_SWORD_REACH * tile_px;
        float sword_tip_x = fighter->facing == FACING_RIGHT ? sword_base_x + sword_len : sword_base_x - sword_len;
        float sword_left = sword_base_x < sword_tip_x ? sword_base_x : sword_tip_x;
        float sword_width = fabsf(sword_tip_x - sword_base_x);
        SDL_FRect sword = {sword_left, sword_y, sword_width, 4.0f};
        SDL_RenderFillRect(renderer, &sword);
    }

    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    SDL_FRect dot = {
        px + (fighter->facing == FACING_LEFT ? 2.0f : PLAYER_WIDTH * tile_px - 7.0f),
        py + 3.0f,
        4.0f,
        4.0f
    };
    SDL_RenderFillRect(renderer, &dot);
}

static void render_camera_view(const GameState *state, const SDL_Rect *viewport) {
    SDL_Renderer *renderer = engine_renderer_get();
    const Arena *arena = &state->arena;
    float tile_px = (float)TILE_SIZE_PX;
    float visible_tiles = (float)viewport->w / tile_px;
    float max_camera_left = (float)arena->width - visible_tiles;
    const FighterState *p1 = &state->combat.fighters[PLAYER_ONE];
    const FighterState *p2 = &state->combat.fighters[PLAYER_TWO];
    float focus_x = (p1->position.x + p2->position.x) * 0.5f;
    if (!p1->alive && p2->alive) {
        focus_x = p2->position.x;
    } else if (p1->alive && !p2->alive) {
        focus_x = p1->position.x;
    }
    float camera_left = clamp_camera_left(
        focus_x + (PLAYER_WIDTH * 0.5f) - (visible_tiles * 0.5f),
        max_camera_left > 0.0f ? max_camera_left : 0.0f);
    int segment_index = (int)state->active_segment;
    int start_tile = (int)camera_left;
    int end_tile = (int)(camera_left + visible_tiles) + 2;
    float world_offset_y = 90.0f;
    Uint8 red = 20;
    Uint8 green = 24;
    Uint8 blue = 34;
    char label[48];

    segment_color(segment_index, &red, &green, &blue);
    SDL_SetRenderViewport(renderer, viewport);
    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_FRect bg = {0.0f, 0.0f, (float)viewport->w, (float)viewport->h};
    SDL_RenderFillRect(renderer, &bg);

    if (end_tile > (int)arena->width) {
        end_tile = (int)arena->width;
    }

    for (int ty = 0; ty < arena->height; ++ty) {
        for (int tx = start_tile; tx < end_tile; ++tx) {
            TileType tile = arena->tiles[ty * arena->width + tx];
            if (tile == TILE_EMPTY) {
                continue;
            }

            switch (tile) {
            case TILE_SOLID: SDL_SetRenderDrawColor(renderer, 84, 82, 75, 255); break;
            case TILE_PLATFORM: SDL_SetRenderDrawColor(renderer, 143, 101, 60, 255); break;
            case TILE_SPAWN_P1: SDL_SetRenderDrawColor(renderer, 32, 98, 214, 255); break;
            case TILE_SPAWN_P2: SDL_SetRenderDrawColor(renderer, 214, 62, 42, 255); break;
            case TILE_HAZARD: SDL_SetRenderDrawColor(renderer, 212, 120, 20, 255); break;
            default: SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); break;
            }

            SDL_FRect tile_rect = {
                ((float)tx - camera_left) * tile_px,
                (float)ty * tile_px + world_offset_y,
                tile_px,
                tile_px
            };
            SDL_RenderFillRect(renderer, &tile_rect);
        }
    }

    if (segment_index == 0) {
        SDL_SetRenderDrawColor(renderer, 240, 205, 90, 255);
        SDL_RenderDebugText(renderer, 14.0f, 18.0f, "P2 BASE");
    }
    if (segment_index == (int)MAP_TOTAL_SEGMENTS - 1) {
        SDL_SetRenderDrawColor(renderer, 240, 205, 90, 255);
        SDL_RenderDebugText(renderer, (float)viewport->w - 70.0f, 18.0f, "P1 BASE");
    }

    snprintf(label, sizeof(label), "ZONE %d / %d", state->active_segment + 1, MAP_TOTAL_SEGMENTS);
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
    SDL_RenderDebugText(renderer, 14.0f, 42.0f, label);

    draw_fighter(renderer, &state->combat.fighters[PLAYER_ONE], 34, 104, 220,
                 camera_left, (float)viewport->w, tile_px, world_offset_y);
    draw_fighter(renderer, &state->combat.fighters[PLAYER_TWO], 220, 52, 42,
                 camera_left, (float)viewport->w, tile_px, world_offset_y);

    for (int owner = 0; owner < 2; ++owner) {
        if (!state->combat.sword_on_ground[owner]) {
            continue;
        }
        float sx = (state->combat.sword_drop_position[owner].x - camera_left) * tile_px;
        float sy = state->combat.sword_drop_position[owner].y * tile_px + world_offset_y;
        if (sx < -12.0f || sx > (float)viewport->w + 12.0f) {
            continue;
        }
        SDL_SetRenderDrawColor(renderer, 245, 235, 160, 255);
        SDL_FRect dropped = {sx - 6.0f, sy - 2.0f, 12.0f, 3.0f};
        SDL_RenderFillRect(renderer, &dropped);
    }

    for (int owner = 0; owner < 2; ++owner) {
        if (!state->combat.sword_in_flight[owner]) {
            continue;
        }
        float sx = (state->combat.sword_flight_position[owner].x - camera_left) * tile_px;
        float sy = state->combat.sword_flight_position[owner].y * tile_px + world_offset_y;
        if (sx < -16.0f || sx > (float)viewport->w + 16.0f) {
            continue;
        }

        SDL_SetRenderDrawColor(renderer, 255, 245, 180, 255);
        SDL_FRect flight = {sx - 8.0f, sy - 1.5f, 16.0f, 3.0f};
        SDL_RenderFillRect(renderer, &flight);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect border = {0.0f, 0.0f, (float)viewport->w, (float)viewport->h};
    SDL_RenderRect(renderer, &border);
}

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

GameError render_frame(const GameState *state, float interpolation_factor) {
    if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

    render_begin();

    // Dessiner selon l’état du jeu
    switch (state->game_phase) {
        case GAME_PHASE_MAIN_MENU:
        case GAME_PHASE_PRESS_START:
        case GAME_PHASE_MAP_MENU:
        case GAME_PHASE_MAP_GENERATE:
        case GAME_PHASE_MAP_RESULTS:
        case GAME_PHASE_MODE_SELECT:
        case GAME_PHASE_OPTIONS:
            menu_render(state);
            break;

        case GAME_PHASE_MATCH:
            render_world(state, interpolation_factor);
            hud_render(state);
            break;

        case GAME_PHASE_GAME_OVER: {
            render_world(state, interpolation_factor);
            hud_render(state);
            SDL_Renderer *r = engine_renderer_get();
            if (r) {
                char text[96];
                float bx = (float)WINDOW_W * 0.5f - 220.0f;
                float by = (float)WINDOW_H * 0.5f - 170.0f;
                SDL_SetRenderDrawColor(r, 15, 15, 15, 240);
                SDL_FRect box = {bx, by, 440.0f, 310.0f};
                SDL_RenderFillRect(r, &box);
                SDL_SetRenderDrawColor(r, 255, 220, 50, 255);
                SDL_RenderDebugText(r, bx + 152.0f, by + 18.0f, "MATCH RESULT");
                snprintf(text, sizeof(text), "Winner: Player %d", state->round_winner + 1);
                SDL_RenderDebugText(r, bx + 24.0f, by + 58.0f, text);
                snprintf(text, sizeof(text), "P1 kills/deaths: %u/%u", state->combat.kill_count[PLAYER_ONE], state->combat.death_count[PLAYER_ONE]);
                SDL_RenderDebugText(r, bx + 24.0f, by + 86.0f, text);
                snprintf(text, sizeof(text), "P2 kills/deaths: %u/%u", state->combat.kill_count[PLAYER_TWO], state->combat.death_count[PLAYER_TWO]);
                SDL_RenderDebugText(r, bx + 24.0f, by + 112.0f, text);
                snprintf(text, sizeof(text), "Score: %u - %u", state->combat.score[PLAYER_ONE], state->combat.score[PLAYER_TWO]);
                SDL_RenderDebugText(r, bx + 24.0f, by + 138.0f, text);
                snprintf(text, sizeof(text), "Thrust kills P1/P2: %u/%u", state->combat.thrust_kill_count[PLAYER_ONE], state->combat.thrust_kill_count[PLAYER_TWO]);
                SDL_RenderDebugText(r, bx + 24.0f, by + 164.0f, text);
                snprintf(text, sizeof(text), "Throw kills P1/P2: %u/%u", state->combat.throw_kill_count[PLAYER_ONE], state->combat.throw_kill_count[PLAYER_TWO]);
                SDL_RenderDebugText(r, bx + 24.0f, by + 190.0f, text);
                snprintf(text, sizeof(text), "Disarms P1/P2: %u/%u", state->combat.disarm_count[PLAYER_ONE], state->combat.disarm_count[PLAYER_TWO]);
                SDL_RenderDebugText(r, bx + 24.0f, by + 216.0f, text);
                snprintf(text, sizeof(text), "Neck snaps P1/P2: %u/%u", state->combat.neck_snap_count[PLAYER_ONE], state->combat.neck_snap_count[PLAYER_TWO]);
                SDL_RenderDebugText(r, bx + 24.0f, by + 242.0f, text);
                snprintf(text, sizeof(text), "Zones crossed P1/P2: %u/%u", state->combat.zones_crossed[PLAYER_ONE], state->combat.zones_crossed[PLAYER_TWO]);
                SDL_RenderDebugText(r, bx + 24.0f, by + 268.0f, text);
                snprintf(text, sizeof(text), "Time: %02u:%02u", (state->combat.round_time_frames / 60U) / 60U,
                         (state->combat.round_time_frames / 60U) % 60U);
                SDL_RenderDebugText(r, bx + 300.0f, by + 58.0f, text);
            }
            break;
        }

        default:
            break;
    }

    render_end();
    return GAME_OK;
}

GameError render_world(const GameState *state, float interpolation_factor) {
    (void)interpolation_factor;
    if (state == NULL) return GAME_ERROR_INVALID_ARGUMENT;

    SDL_Renderer *renderer = engine_renderer_get();
    if (renderer == NULL) return GAME_ERROR_INVALID_STATE;
    if (state->arena.tiles == NULL) return GAME_OK;

    SDL_Rect full_view = {0, HUD_HEIGHT_PX, WINDOW_W, WINDOW_H - HUD_HEIGHT_PX};
    render_camera_view(state, &full_view);
	SDL_SetRenderViewport(renderer, NULL);

	if (state->match_phase == MATCH_PHASE_COUNTDOWN) {
		char text[16];
		int number = (int)((MATCH_COUNTDOWN_FRAMES - state->phase_elapsed - 1U) / 30U) + 1;
		snprintf(text, sizeof(text), "%d", number);
		SDL_SetRenderDrawColor(renderer, 255, 230, 120, 255);
		SDL_RenderDebugText(renderer, (float)WINDOW_W * 0.5f - 4.0f, (float)WINDOW_H * 0.5f, text);
	}

    if (state->combat.block_spark_frames > 0) {
        float sx = state->combat.block_spark_position.x * TILE_SIZE_PX;
        float sy = state->combat.block_spark_position.y * TILE_SIZE_PX + 90.0f;
        SDL_SetRenderDrawColor(renderer, 255, 245, 150, 255);
        SDL_FRect h = {sx - 8.0f, sy - 1.0f, 16.0f, 2.0f};
        SDL_FRect v = {sx - 1.0f, sy - 8.0f, 2.0f, 16.0f};
        SDL_RenderFillRect(renderer, &h);
        SDL_RenderFillRect(renderer, &v);
    }

    if (state->transition_frames > 0) {
        char text[64];
        SDL_SetRenderDrawColor(renderer, 12, 12, 18, 220);
        SDL_FRect box = {(float)WINDOW_W * 0.5f - 180.0f, (float)WINDOW_H * 0.5f - 42.0f, 360.0f, 84.0f};
        SDL_RenderFillRect(renderer, &box);
        SDL_SetRenderDrawColor(renderer, 255, 220, 90, 255);
        SDL_RenderDebugText(renderer, (float)WINDOW_W * 0.5f - 54.0f, (float)WINDOW_H * 0.5f - 16.0f, "TRANSITION");
        snprintf(text, sizeof(text), "Entering zone %u", state->pending_segment + 1);
        SDL_RenderDebugText(renderer, (float)WINDOW_W * 0.5f - 68.0f, (float)WINDOW_H * 0.5f + 10.0f, text);
    }

    return GAME_OK;
}
