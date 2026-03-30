#include <math.h>

#include "core/constants.h"
#include "render/render.h"
#include "render/animation.h"
#include "render/assets.h"
#include "render_internal.h"
#include "utils/utils.h"
#include "utils/log.h"

static bool fighter_is_attack_action(FighterAction action)
{
    return action == FIGHTER_ACTION_ATTACK_HIGH ||
           action == FIGHTER_ACTION_ATTACK_MID ||
           action == FIGHTER_ACTION_ATTACK_LOW;
}

static SDL_Renderer* render_get_renderer(Engine* engine)
{
    return (SDL_Renderer*)engine_get_renderer_handle(engine);
}

bool render_init(RenderContext* render, Engine* engine)
{
    SDL_Renderer* renderer;

    if (!render || !engine) {
        return false;
    }

    render->debug_draw = false;
    render->camera.position.x = 0.0f;
    render->camera.position.y = 0.0f;
    render->camera.zoom = 1.0f;
    render->camera_target.x = 0.0f;
    render->camera_target.y = 0.0f;
    render->camera_lerp_speed = 0.15f;  /* Smooth camera tracking */

    /* Initialize asset and animation systems */
    renderer = render_get_renderer(engine);
    if (renderer) {
        if (assets_init(renderer) != GAME_OK) {
            log_warn("render_init: assets_init failed, will use fallback rendering");
        }
    }

    return true;
}

void render_shutdown(RenderContext* render)
{
    (void)render;
    assets_shutdown();
}

void render_update_camera(RenderContext* render, const Game* game)
{
    const Room* room;
    f32 room_width_px;
    f32 center_x;
    f32 max_camera_x;
    i32 focus_fighter_index = -1;
    const Fighter* focus_fighter = NULL;
    const Fighter* enemy_fighter = NULL;
    bool focus_has_room_push_advantage = false;
    f32 target_x;

    if (!render || !game) {
        return;
    }

    room = arena_get_current_room_const(&game->arena);
    if (!room) {
        return;
    }

    room_width_px = (f32)(room->width_tiles * TILE_SIZE);

     /* Determine which fighter to follow:
         Priority 1: The attacker who just killed (if kill was recent)
         Priority 2: Active room-push owner (winner advantage)
         Priority 3: Both fighters' center */
    if (game->combat.kill_happened && game->combat.kill_attacker_index >= 0 &&
        game->combat.kill_attacker_index < MAX_PLAYERS) {
        focus_fighter_index = game->combat.kill_attacker_index;
        focus_fighter = &game->combat.fighters[focus_fighter_index];
        enemy_fighter = &game->combat.fighters[1 - focus_fighter_index];
    } else if (game->combat.kill_attacker_index >= 0 && 
               game->combat.kill_attacker_index < MAX_PLAYERS &&
               game->combat.fighters[game->combat.kill_attacker_index].state.alive) {
        /* Continue following killer for a short window after kill */
        focus_fighter_index = game->combat.kill_attacker_index;
        focus_fighter = &game->combat.fighters[focus_fighter_index];
        enemy_fighter = &game->combat.fighters[1 - focus_fighter_index];
    } else if (game->room_push_direction > 0) {
        focus_fighter_index = 0;
        focus_fighter = &game->combat.fighters[focus_fighter_index];
        enemy_fighter = &game->combat.fighters[1 - focus_fighter_index];
    } else if (game->room_push_direction < 0) {
        focus_fighter_index = 1;
        focus_fighter = &game->combat.fighters[focus_fighter_index];
        enemy_fighter = &game->combat.fighters[1 - focus_fighter_index];
    }

    if (focus_fighter != NULL && focus_fighter->state.alive) {
        /* Follow focused fighter (killer or room-push owner). */
        center_x = focus_fighter->state.pos.x + PLAYER_WIDTH * 0.5f;

        if (focus_fighter_index >= 0) {
            i32 expected_push_dir = (focus_fighter_index == 0) ? 1 : -1;
            focus_has_room_push_advantage = (game->room_push_direction == expected_push_dir);
        }
        
        /*
           Keep enemy in view only when no active room-push advantage.
           During push, camera must stay on the winner to avoid losing focus.
        */
        if (!focus_has_room_push_advantage && enemy_fighter != NULL && enemy_fighter->state.alive) {
            f32 enemy_x = enemy_fighter->state.pos.x + PLAYER_WIDTH * 0.5f;
            f32 distance = fabsf(enemy_x - center_x);
            
            /* If enemy gets too far, bias camera toward them */
            if (distance > WINDOW_WIDTH * 0.6f) {
                center_x = (center_x + enemy_x) * 0.5f;
            }
        }
    } else {
        /* Standard: follow center of both fighters */
        center_x = (game->combat.fighters[0].state.pos.x + game->combat.fighters[1].state.pos.x) * 0.5f;
        center_x += PLAYER_WIDTH * 0.5f;
    }

    max_camera_x = room_width_px - (f32)WINDOW_WIDTH;
    clamp_min_zero_f32(&max_camera_x);

    target_x = clampf(center_x - (f32)WINDOW_WIDTH * 0.5f, 0.0f, max_camera_x);
    render->camera_target.x = target_x;
    render->camera_target.y = 0.0f;

    /* Smooth camera tracking (lerp toward target) */
    render->camera.position.x = 
        render->camera.position.x + 
        (render->camera_target.x - render->camera.position.x) * render->camera_lerp_speed;
    render->camera.position.y = 0.0f;
}

void render_draw_room(RenderContext* render, Engine* engine, const Room* room)
{
    SDL_Renderer* renderer;
    i32 y;
    i32 x;

    if (!render || !engine || !room) {
        return;
    }

    renderer = render_get_renderer(engine);
    if (!renderer) {
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
                SDL_SetRenderDrawColor(renderer, 62, 62, 70, 255);
            } else if (tile == TILE_PLATFORM) {
                SDL_SetRenderDrawColor(renderer, 95, 95, 110, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 150, 40, 40, 255);
            }

            SDL_RenderFillRect(renderer, &r);
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
    SDL_Renderer* renderer;
    SDL_FRect line_rect;
    f32 thrust_ratio;
    f32 extend_ratio;
    f32 attack_target_width;
    f32 x = fighter->state.pos.x - render->camera.position.x;
    f32 y = fighter->state.pos.y - render->camera.position.y;

    renderer = render_get_renderer(engine);
    if (!renderer) {
        return;
    }

    if (!fighter->state.has_sword || fighter->state.action == FIGHTER_ACTION_RUN) {
        return;
    }

    line_rect.w = SWORD_RENDER_BASE_WIDTH;
    line_rect.h = SWORD_RENDER_BASE_HEIGHT;

    if (fighter->state.has_sword && fighter_is_attack_action(fighter->state.action) &&
        fighter->state.action_duration > 0.0f) {
        thrust_ratio = clampf(
            fighter->state.action_time / fighter->state.action_duration,
            0.0f,
            1.0f
        );
        extend_ratio = clampf(thrust_ratio / THRUST_RENDER_EXTEND_FRACTION, 0.0f, 1.0f);
        attack_target_width = SWORD_THRUST_RANGE_X * ATTACK_RANGE_ARMED_MULTIPLIER;
        line_rect.w = SWORD_RENDER_BASE_WIDTH + (attack_target_width - SWORD_RENDER_BASE_WIDTH) * extend_ratio;
        line_rect.h = SWORD_RENDER_THRUST_HEIGHT;
    }

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

    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &line_rect);
}

static void render_draw_thrown_sword(RenderContext* render, Engine* engine, const CombatState* combat)
{
    SDL_Renderer* renderer;
    const ThrownSword* sword;
    SDL_FRect dst;
    SDL_Texture* tex;
    f32 life_ratio;

    if (!render || !engine || !combat) {
        return;
    }

    sword = &combat->thrown_sword;
    if (!sword->active) {
        return;
    }

    renderer = render_get_renderer(engine);
    if (!renderer) {
        return;
    }

    dst.w = THROW_PROJECTILE_WIDTH;
    dst.h = THROW_PROJECTILE_HEIGHT;
    dst.x = sword->pos.x - render->camera.position.x - dst.w * 0.5f;
    dst.y = sword->pos.y - render->camera.position.y - dst.h * 0.5f;

    tex = assets_get_texture(ASSET_TEX_FLYING_SWORD);
    if (!tex) {
        life_ratio = 1.0f - clampf(sword->lifetime / THROW_PROJECTILE_LIFETIME, 0.0f, 1.0f);
        tex = animation_get_throw_projectile_texture(life_ratio);
    }

    if (tex) {
        SDL_FPoint center = { dst.w * 0.5f, dst.h * 0.5f };
        SDL_RenderTextureRotated(renderer, tex, NULL, &dst, sword->rotation_deg, &center, SDL_FLIP_NONE);
    } else {
        SDL_SetRenderDrawColor(renderer, 235, 235, 225, 255);
        SDL_RenderFillRect(renderer, &dst);
    }
}

void render_draw_fighters(RenderContext* render, Engine* engine, const Game* game)
{
    SDL_Renderer* renderer;
    const Fighter* p1 = &game->combat.fighters[0];
    const Fighter* p2 = &game->combat.fighters[1];
    SDL_Texture* body_tex;
    SDL_Texture* ovl_tex;

    renderer = render_get_renderer(engine);
    if (!renderer) {
        return;
    }

    /* Update animations */
    if (p1->state.alive) {
        animation_update(0, &p1->state);
    }
    if (p2->state.alive) {
        animation_update(1, &p2->state);
    }

    /* Draw player 1 */
    if (p1->state.alive) {
        SDL_FRect r1;
        r1.x = p1->state.pos.x - render->camera.position.x;
        r1.y = p1->state.pos.y - render->camera.position.y;
        r1.w = PLAYER_WIDTH;
        r1.h = PLAYER_HEIGHT;

        /* Try to draw textured sprite, fallback to rectangle */
        body_tex = animation_get_body_texture(0, &p1->state);
        if (body_tex) {
            SDL_RenderTextureRotated(
                renderer, body_tex, NULL, &r1, 0.0f,
                NULL, animation_flip(&p1->state)
            );
        } else {
            /* Fallback: solid rectangle */
            SDL_SetRenderDrawColor(renderer, 255, 176, 43, 255);
            SDL_RenderFillRect(renderer, &r1);
        }

        /* Draw sword overlay if applicable */
        ovl_tex = animation_get_ovl_texture(0, &p1->state);
        if (ovl_tex) {
            SDL_RenderTextureRotated(
                renderer, ovl_tex, NULL, &r1, 0.0f,
                NULL, animation_flip(&p1->state)
            );
        }

        draw_sword_line_indicator(render, engine, p1, 255, 235, 190);
    }

    /* Draw player 2 */
    if (p2->state.alive) {
        SDL_FRect r2;
        r2.x = p2->state.pos.x - render->camera.position.x;
        r2.y = p2->state.pos.y - render->camera.position.y;
        r2.w = PLAYER_WIDTH;
        r2.h = PLAYER_HEIGHT;

        /* Try to draw textured sprite, fallback to rectangle */
        body_tex = animation_get_body_texture(1, &p2->state);
        if (body_tex) {
            SDL_RenderTextureRotated(
                renderer, body_tex, NULL, &r2, 0.0f,
                NULL, animation_flip(&p2->state)
            );
        } else {
            /* Fallback: solid rectangle */
            SDL_SetRenderDrawColor(renderer, 72, 170, 255, 255);
            SDL_RenderFillRect(renderer, &r2);
        }

        /* Draw sword overlay if applicable */
        ovl_tex = animation_get_ovl_texture(1, &p2->state);
        if (ovl_tex) {
            SDL_RenderTextureRotated(
                renderer, ovl_tex, NULL, &r2, 0.0f,
                NULL, animation_flip(&p2->state)
            );
        }

        draw_sword_line_indicator(render, engine, p2, 190, 216, 255);
    }
}

void render_frame(RenderContext* render, Engine* engine, const Game* game)
{
    SDL_Renderer* renderer;
    const Room* room;


    if (!render || !engine || !game) {
        return;
    }

    renderer = render_get_renderer(engine);
    if (!renderer) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 24, 24, 31, 255);
    SDL_RenderClear(renderer);

    /* Render menu if in menu state */
    if (game->phase == GAME_PHASE_MENU) {
        render_menu(renderer, (MenuContext*)&game->menu);
        SDL_RenderPresent(renderer);
        return;
    }

    /* Render pause menu on top of game */
    if (game->phase == GAME_PHASE_PAUSED) {
        /* Render game in background (darkened) */
        room = arena_get_current_room_const(&game->arena);
        if (room) {
            render_update_camera(render, game);
            render_draw_room(render, engine, room);
            render_draw_fighters(render, engine, game);
            render_draw_thrown_sword(render, engine, &game->combat);
        }

        /* Darken the screen */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_FRect overlay = {0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_RenderFillRect(renderer, &overlay);

        /* Render pause menu */
        render_menu(renderer, (MenuContext*)&game->menu);
        SDL_RenderPresent(renderer);
        return;
    }

    /* Render victory screen */
    if (game->phase == GAME_PHASE_VICTORY) {
        render_victory_screen(renderer, game, game->victory_winner_index);
        SDL_RenderPresent(renderer);
        return;
    }

    /* Render game otherwise */
    room = arena_get_current_room_const(&game->arena);
    if (!room) {
        return;
    }

    render_update_camera(render, game);

    render_draw_room(render, engine, room);
    render_draw_fighters(render, engine, game);
    render_draw_thrown_sword(render, engine, &game->combat);

    SDL_RenderPresent(renderer);
}
