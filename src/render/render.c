#include <math.h>

#include "core/constants.h"
#include "render/render.h"
#include "assets_private.h"
#include "render_internal.h"
#include "utils/utils.h"
#include "utils/log.h"

static SDL_Texture *fighter_sprite_texture(const FighterState *state)
{
    if (!state)
        return NULL;
    i32 animation = -1;
    bool loop = false;
    switch (state->action)
    {
    case FIGHTER_ACTION_CROUCH:
        animation = 13;
        loop = true;
        break;
    case FIGHTER_ACTION_DIVE_KICK:
        animation = 14;
        break;
    case FIGHTER_ACTION_SWEEP:
        animation = 15;
        break;
    case FIGHTER_ACTION_KNOCKED_DOWN:
        animation = 16;
        break;
    case FIGHTER_ACTION_WALL_SLIDE:
        animation = 17;
        loop = true;
        break;
    case FIGHTER_ACTION_RUN:
        animation = 0;
        loop = true;
        break;
    case FIGHTER_ACTION_JUMP:
        animation = 1;
        break;
    case FIGHTER_ACTION_FALL:
        animation = 2;
        break;
    case FIGHTER_ACTION_ROLL:
        animation = 3;
        break;
    case FIGHTER_ACTION_ATTACK_HIGH:
        animation = state->has_sword ? 4 : 7;
        break;
    case FIGHTER_ACTION_ATTACK_MID:
        animation = state->has_sword ? 5 : 7;
        break;
    case FIGHTER_ACTION_ATTACK_LOW:
        animation = state->has_sword ? 6 : 7;
        break;
    case FIGHTER_ACTION_STUN:
        animation = 8;
        break;
    case FIGHTER_ACTION_DEAD:
        animation = 9;
        break;
    case FIGHTER_ACTION_THROW:
        animation = 10;
        break;
    default:
        if (state->has_sword && state->sword_line != SWORD_LINE_MID)
            animation = state->sword_line == SWORD_LINE_HIGH ? 11 : 12;
        break;
    }
    f32 time = state->action_duration > 0 ? state->action_time : state->animation_time;
    SDL_Texture *tex = assets_animation(animation, time, loop);
    return tex ? tex
               : assets_get_texture(state->has_sword ? ASSET_TEX_FIGHTER_SWORD
                                                     : ASSET_TEX_FIGHTER_UNARMED);
}

static SDL_FlipMode fighter_flip(const FighterState *state)
{
    if (!state || state->facing_right)
    {
        return SDL_FLIP_NONE;
    }
    return SDL_FLIP_HORIZONTAL;
}

static SDL_Renderer *render_get_renderer(Engine *engine)
{
    return (SDL_Renderer *)engine_get_renderer_handle(engine);
}

bool render_init(RenderContext *render, Engine *engine)
{
    SDL_Renderer *renderer;

    if (!render || !engine)
    {
        return false;
    }

    render->last_camera_tick = 0;
    render->last_room = -1;
    render->debug_draw = false;
    render->camera.position.x = 0.0f;
    render->camera.position.y =
        fmaxf(0, ROOM_HEIGHT_TILES * TILE_SIZE - WINDOW_HEIGHT / render->camera.zoom);
    render->camera.zoom = 2.0f;
    render->camera_target.x = 0.0f;
    render->camera_target.y =
        fmaxf(0, ROOM_HEIGHT_TILES * TILE_SIZE - WINDOW_HEIGHT / render->camera.zoom);
    render->camera_lerp_speed = 0.15f;

    renderer = render_get_renderer(engine);
    if (renderer)
    {
        if (assets_init(renderer) != GAME_OK)
        {
            log_warn("render_init: assets_init failed, will use fallback rendering");
        }
    }

    return true;
}

void render_shutdown(RenderContext *render)
{
    (void)render;
    assets_shutdown();
}

void render_update_camera(RenderContext *render, const Game *game)
{
    const Room *room;
    f32 room_width_px;
    f32 center_x;
    f32 max_camera_x;
    i32 focus_fighter_index = -1;
    const Fighter *focus_fighter = NULL;
    const Fighter *enemy_fighter = NULL;
    bool focus_has_room_push_advantage = false;
    f32 target_x;

    if (!render || !game)
    {
        return;
    }

    room = arena_get_current_room_const(&game->arena);
    if (!room)
    {
        return;
    }

    room_width_px = (f32)(room->width_tiles * TILE_SIZE);

    if (game->combat.kill_happened && game->combat.kill_attacker_index >= 0 &&
        game->combat.kill_attacker_index < MAX_PLAYERS)
    {
        focus_fighter_index = game->combat.kill_attacker_index;
        focus_fighter = &game->combat.fighters[focus_fighter_index];
        enemy_fighter = &game->combat.fighters[1 - focus_fighter_index];
    }
    else if (game->combat.kill_attacker_index >= 0 &&
             game->combat.kill_attacker_index < MAX_PLAYERS &&
             game->combat.fighters[game->combat.kill_attacker_index].state.alive)
    {
        focus_fighter_index = game->combat.kill_attacker_index;
        focus_fighter = &game->combat.fighters[focus_fighter_index];
        enemy_fighter = &game->combat.fighters[1 - focus_fighter_index];
    }
    else if (game->room_push_direction > 0)
    {
        focus_fighter_index = 0;
        focus_fighter = &game->combat.fighters[focus_fighter_index];
        enemy_fighter = &game->combat.fighters[1 - focus_fighter_index];
    }
    else if (game->room_push_direction < 0)
    {
        focus_fighter_index = 1;
        focus_fighter = &game->combat.fighters[focus_fighter_index];
        enemy_fighter = &game->combat.fighters[1 - focus_fighter_index];
    }

    if (focus_fighter != NULL && focus_fighter->state.alive)
    {
        center_x = focus_fighter->state.pos.x + PLAYER_WIDTH * 0.5f;

        if (focus_fighter_index >= 0)
        {
            i32 expected_push_dir = (focus_fighter_index == 0) ? 1 : -1;
            focus_has_room_push_advantage = (game->room_push_direction == expected_push_dir);
        }

        if (!focus_has_room_push_advantage && enemy_fighter != NULL && enemy_fighter->state.alive)
        {
            f32 enemy_x = enemy_fighter->state.pos.x + PLAYER_WIDTH * 0.5f;
            f32 distance = fabsf(enemy_x - center_x);

            if (distance > WINDOW_WIDTH * 0.6f)
            {
                center_x = (center_x + enemy_x) * 0.5f;
            }
        }
    }
    else
    {
        center_x =
            (game->combat.fighters[0].state.pos.x + game->combat.fighters[1].state.pos.x) * 0.5f;
        center_x += PLAYER_WIDTH * 0.5f;
    }

    max_camera_x = room_width_px - (f32)WINDOW_WIDTH / render->camera.zoom;
    clamp_min_zero_f32(&max_camera_x);

    target_x = clampf(center_x - (f32)WINDOW_WIDTH / (2 * render->camera.zoom), 0.0f, max_camera_x);
    render->camera_target.x = target_x;
    render->camera_target.y =
        fmaxf(0, ROOM_HEIGHT_TILES * TILE_SIZE - WINDOW_HEIGHT / render->camera.zoom);

    u64 tick = SDL_GetTicks();
    f32 elapsed = render->last_camera_tick
                      ? fminf((tick - render->last_camera_tick) / 1000.0f, 0.1f)
                      : 1.0f / 60.0f;
    render->last_camera_tick = tick;
    f32 blend = 1.0f - powf(1.0f - render->camera_lerp_speed, elapsed * 60.0f);
    if (render->last_room != game->arena.current_room)
    {
        render->camera.position.x = target_x;
        render->last_room = game->arena.current_room;
    }
    render->camera.position.x =
        render->camera.position.x + (render->camera_target.x - render->camera.position.x) * blend;
    render->camera.position.y =
        fmaxf(0, ROOM_HEIGHT_TILES * TILE_SIZE - WINDOW_HEIGHT / render->camera.zoom);
}

static void decor_rect(SDL_Renderer *renderer, f32 x, f32 y, f32 w, f32 h, u8 red, u8 green,
                       u8 blue)
{
    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_FRect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

/* Original procedural pixel scenery: palette and silhouettes reference the genre,
 * while collision tiles remain the sole source of traversable geometry. */
static void render_backdrop(SDL_Renderer *renderer, const Room *room, f32 camera)
{
    if (room->theme == ARENA_THEME_CASTLE)
    {
        decor_rect(renderer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 111, 49, 31);
        for (i32 i = 0; i < 14; i++)
        {
            f32 x = i * 112 - (i32)(camera * 0.25f) % 112;
            decor_rect(renderer, x, 60, 56, 520, 132, 65, 36);
            decor_rect(renderer, x + 7, 70, 42, 500, 148, 79, 39);
            decor_rect(renderer, x - 10, 60, 76, 16, 204, 139, 62);
            for (i32 y = 120; y < 570; y += 60)
                decor_rect(renderer, x + 5, (f32)y, 46, 2, 116, 50, 31);
        }
        for (i32 i = 0; i < 9; i++)
            decor_rect(renderer, i * 160 + 20, 180, 12, 120, 55, 24, 30);
    }
    else if (room->theme == ARENA_THEME_WILDS)
    {
        decor_rect(renderer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 58, 25, 43);
        for (i32 i = 0; i < 12; i++)
        {
            f32 x = i * 128 - (i32)(camera * 0.3f) % 128;
            decor_rect(renderer, x + 45, 100, 18, 540, 72, 38, 45);
            for (i32 j = 0; j < 6; j++)
                decor_rect(renderer, x + j * 15, 80 + (j % 3) * 22, 68, 76, 105 + j * 5, 35, 47);
        }
    }
    else if (room->theme == ARENA_THEME_MINES)
    {
        decor_rect(renderer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 24, 24, 29);
        for (i32 i = 0; i < 11; i++)
        {
            f32 x = i * 140 - (i32)(camera * 0.3f) % 140;
            decor_rect(renderer, x, 90, 80, 360, 37, 33, 42);
            decor_rect(renderer, x + 16, 100, 10, 540, 64, 52, 56);
            decor_rect(renderer, x - 10, 140, 110, 12, 75, 57, 51);
            decor_rect(renderer, x + 74, 210, 8, 13, 215, 128, 58);
        }
    }
    else
    {
        decor_rect(renderer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 53, 72, 123);
        for (i32 i = 0; i < 14; i++)
        {
            f32 x = i * 100 - (i32)(camera * 0.18f) % 100;
            decor_rect(renderer, x, 110 + (i % 4) * 85, 72, 15, 137, 155, 188);
            decor_rect(renderer, x + 15, 100 + (i % 4) * 85, 44, 14, 166, 182, 203);
        }
    }
}

void render_draw_room(RenderContext *render, Engine *engine, const Room *room)
{
    SDL_Renderer *renderer;
    i32 y;
    i32 x;

    if (!render || !engine || !room)
    {
        return;
    }

    renderer = render_get_renderer(engine);
    if (!renderer)
    {
        return;
    }

    render_backdrop(renderer, room, render->camera.position.x);

    for (y = 0; y < room->height_tiles; ++y)
    {
        for (x = 0; x < room->width_tiles; ++x)
        {
            TileType tile = room->tiles[y][x];
            SDL_FRect r;

            if (tile == TILE_EMPTY)
            {
                continue;
            }

            r.x = (f32)(x * TILE_SIZE) - render->camera.position.x;
            r.y = (f32)(y * TILE_SIZE) - render->camera.position.y;
            r.w = (f32)TILE_SIZE;
            r.h = (f32)TILE_SIZE;

            u8 red = 215, green = 158, blue = 71;
            if (room->theme == ARENA_THEME_WILDS)
            {
                red = 137;
                green = 136;
                blue = 27;
            }
            if (room->theme == ARENA_THEME_MINES)
            {
                red = 100;
                green = 84;
                blue = 72;
            }
            if (room->theme == ARENA_THEME_CLOUDS || tile == TILE_CLOUD)
            {
                red = 224;
                green = 223;
                blue = 231;
            }
            if (tile == TILE_HAZARD)
            {
                red = 177;
                green = 37;
                blue = 46;
            }
            SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
            SDL_RenderFillRect(renderer, &r);
            decor_rect(renderer, r.x, r.y, TILE_SIZE, 3, (u8)fminf(255, red + 20),
                       (u8)fminf(255, green + 20), blue);
            if (tile != TILE_CLOUD)
            {
                decor_rect(renderer, r.x + ((x + y) % 3) * 5, r.y + 15, 15, 2, red * 0.8f,
                           green * 0.8f, blue * 0.8f);
                if (room->theme == ARENA_THEME_WILDS && room_get_tile(room, x, y - 1) == TILE_EMPTY)
                    for (i32 grass = 0; grass < 4; grass++)
                        decor_rect(renderer, r.x + grass * 8, r.y - 4 - (x + grass) % 5, 3, 8, 154,
                                   153, 31);
            }
        }
    }
}

static void draw_sword_line_indicator(RenderContext *render, Engine *engine, const Fighter *fighter,
                                      u8 r, u8 g, u8 b)
{
    SDL_Renderer *renderer;
    SDL_FRect line_rect;
    f32 x = fighter->state.pos.x - render->camera.position.x;
    f32 y = fighter->state.pos.y - render->camera.position.y;

    renderer = render_get_renderer(engine);
    if (!renderer)
    {
        return;
    }

    if (!fighter->state.has_sword || fighter->state.action == FIGHTER_ACTION_RUN)
    {
        return;
    }

    line_rect.w = combat_sword_reach(fighter);
    line_rect.h = 1.5f;
    if (line_rect.w <= 0)
        return;

    if (fighter->state.sword_line == SWORD_LINE_HIGH)
    {
        line_rect.y = y + 8.0f;
    }
    else if (fighter->state.sword_line == SWORD_LINE_LOW)
    {
        line_rect.y = y + PLAYER_HEIGHT - 12.0f;
    }
    else
    {
        line_rect.y = y + PLAYER_HEIGHT * 0.5f - 2.0f;
    }

    if (fighter->state.facing_right)
    {
        line_rect.x = x + PLAYER_WIDTH;
    }
    else
    {
        line_rect.x = x - line_rect.w;
    }

    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &line_rect);
}

static void render_draw_thrown_sword(RenderContext *render, Engine *engine,
                                     const CombatState *combat)
{
    SDL_Renderer *renderer;
    const ThrownSword *sword;
    SDL_FRect dst;
    SDL_Texture *tex;

    if (!render || !engine || !combat)
    {
        return;
    }

    renderer = render_get_renderer(engine);
    if (!renderer)
        return;
    for (i32 i = 0; i < MAX_PLAYERS; i++)
        if (combat->ground_swords[i].active)
        {
            SDL_FRect ground = {combat->ground_swords[i].pos.x - render->camera.position.x,
                                combat->ground_swords[i].pos.y + PLAYER_HEIGHT - 5 -
                                    render->camera.position.y,
                                24, 4};
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
            SDL_RenderFillRect(renderer, &ground);
        }
    for (i32 projectile = 0; projectile < 2; projectile++)
    {
        sword = projectile == 0 ? &combat->thrown_sword : &combat->second_thrown_sword;
        if (!sword->active)
        {
            continue;
        }

        renderer = render_get_renderer(engine);
        if (!renderer)
        {
            continue;
        }

        dst.w = THROW_PROJECTILE_WIDTH;
        dst.h = THROW_PROJECTILE_HEIGHT;
        dst.x = sword->pos.x - render->camera.position.x - dst.w * 0.5f;
        dst.y = sword->pos.y - render->camera.position.y - dst.h * 0.5f;

        tex = assets_get_texture(ASSET_TEX_FLYING_SWORD);

        if (tex)
        {
            SDL_FPoint center = {dst.w * 0.5f, dst.h * 0.5f};
            SDL_RenderTextureRotated(renderer, tex, NULL, &dst, sword->rotation_deg, &center,
                                     SDL_FLIP_NONE);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 235, 235, 225, 255);
            SDL_RenderFillRect(renderer, &dst);
        }
    }
}

void render_draw_fighters(RenderContext *render, Engine *engine, const Game *game)
{
    SDL_Renderer *renderer;
    const Fighter *p1 = &game->combat.fighters[0];
    const Fighter *p2 = &game->combat.fighters[1];
    SDL_Texture *body_tex;

    renderer = render_get_renderer(engine);
    if (!renderer)
    {
        return;
    }

    {
        SDL_FRect r1;
        r1.x = p1->state.pos.x - render->camera.position.x;
        r1.y = p1->state.pos.y - render->camera.position.y;
        if (p1->state.crouching || p1->state.knocked_down ||
            p1->state.action == FIGHTER_ACTION_ROLL)
            r1.y += PLAYER_HEIGHT / 2;
        r1.w = PLAYER_WIDTH;
        r1.h =
            p1->state.crouching || p1->state.knocked_down || p1->state.action == FIGHTER_ACTION_ROLL
                ? PLAYER_HEIGHT / 2
                : PLAYER_HEIGHT;

        body_tex = fighter_sprite_texture(&p1->state);
        if (body_tex)
        {
            SDL_SetTextureColorMod(body_tex, 255, 235, 0);
            SDL_RenderTextureRotated(renderer, body_tex, NULL, &r1, 0.0f, NULL,
                                     fighter_flip(&p1->state));
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 176, 43, 255);
            SDL_RenderFillRect(renderer, &r1);
        }

        if (!p1->state.alive)
            for (i32 i = 0; i < 18; i++)
            {
                f32 t = fminf(p1->state.animation_time, 0.7f);
                f32 vx = (i % 7 - 3) * 55, vy = -120 - (i % 4) * 40;
                decor_rect(renderer, r1.x + vx * t, r1.y + vy * t + 360 * t * t, 3, 5, 255, 235, 0);
            }
        draw_sword_line_indicator(render, engine, p1, 245, 245, 245);
    }

    {
        SDL_FRect r2;
        r2.x = p2->state.pos.x - render->camera.position.x;
        r2.y = p2->state.pos.y - render->camera.position.y;
        if (p2->state.crouching || p2->state.knocked_down ||
            p2->state.action == FIGHTER_ACTION_ROLL)
            r2.y += PLAYER_HEIGHT / 2;
        r2.w = PLAYER_WIDTH;
        r2.h =
            p2->state.crouching || p2->state.knocked_down || p2->state.action == FIGHTER_ACTION_ROLL
                ? PLAYER_HEIGHT / 2
                : PLAYER_HEIGHT;

        body_tex = fighter_sprite_texture(&p2->state);
        if (body_tex)
        {
            SDL_SetTextureColorMod(body_tex, 255, 142, 43);
            SDL_RenderTextureRotated(renderer, body_tex, NULL, &r2, 0.0f, NULL,
                                     fighter_flip(&p2->state));
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 72, 170, 255, 255);
            SDL_RenderFillRect(renderer, &r2);
        }

        if (!p2->state.alive)
            for (i32 i = 0; i < 18; i++)
            {
                f32 t = fminf(p2->state.animation_time, 0.7f);
                f32 vx = (i % 7 - 3) * 55, vy = -120 - (i % 4) * 40;
                decor_rect(renderer, r2.x + vx * t, r2.y + vy * t + 360 * t * t, 3, 5, 255, 142,
                           43);
            }
        draw_sword_line_indicator(render, engine, p2, 245, 245, 245);
    }
}

void render_frame(RenderContext *render, Engine *engine, const Game *game)
{
    SDL_Renderer *renderer;
    const Room *room;

    if (!render || !engine || !game)
    {
        return;
    }

    renderer = render_get_renderer(engine);
    if (!renderer)
    {
        return;
    }

    SDL_SetRenderScale(renderer, 1, 1);
    SDL_SetRenderDrawColor(renderer, 24, 24, 31, 255);
    SDL_RenderClear(renderer);

    if (game->phase == GAME_PHASE_MENU)
    {
        render_menu(renderer, (MenuContext *)&game->menu);
        SDL_RenderPresent(renderer);
        return;
    }

    if (game->phase == GAME_PHASE_PAUSED)
    {
        room = arena_get_current_room_const(&game->arena);
        if (room)
        {
            SDL_SetRenderScale(renderer, render->camera.zoom, render->camera.zoom);
            render_update_camera(render, game);
            render_draw_room(render, engine, room);
            render_draw_fighters(render, engine, game);
            render_draw_thrown_sword(render, engine, &game->combat);
        }

        SDL_SetRenderScale(renderer, 1, 1);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_FRect overlay = {0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_RenderFillRect(renderer, &overlay);

        render_menu(renderer, (MenuContext *)&game->menu);
        SDL_RenderPresent(renderer);
        return;
    }

    if (game->phase == GAME_PHASE_VICTORY)
    {
        render_victory_screen(renderer, game, game->victory_winner_index);
        SDL_RenderPresent(renderer);
        return;
    }

    room = arena_get_current_room_const(&game->arena);
    if (!room)
    {
        return;
    }

    SDL_SetRenderScale(renderer, render->camera.zoom, render->camera.zoom);
    render_update_camera(render, game);

    render_draw_room(render, engine, room);
    render_draw_fighters(render, engine, game);
    render_draw_thrown_sword(render, engine, &game->combat);
    SDL_SetRenderScale(renderer, 1, 1);
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderDebugTextFormat(renderer, 16, 16, "ROOM %d/%d | SEED %u | %s",
                              game->arena.current_room + 1, game->arena.room_count,
                              game->arena_options.seed,
                              game->room_push_direction > 0   ? "P1 >>>"
                              : game->room_push_direction < 0 ? "<<< P2"
                                                              : "DUEL");
    if (game->room_push_direction)
    {
        bool right = game->room_push_direction > 0;
        f32 x = right ? WINDOW_WIDTH - 150 : 35;
        decor_rect(renderer, x, 65, 110, 36, 255, 230, 25);
        SDL_SetRenderDrawColor(renderer, 35, 20, 20, 255);
        SDL_SetRenderScale(renderer, 2, 2);
        SDL_RenderDebugText(renderer, x / 2 + 7, 38, right ? "GO >>>" : "<<< GO");
        SDL_SetRenderScale(renderer, 1, 1);
    }
    if (render->debug_draw)
        for (i32 i = 0; i < MAX_PLAYERS; i++)
        {
            const FighterState *f = &game->combat.fighters[i].state;
            SDL_FRect body = {f->pos.x - render->camera.position.x, f->pos.y, PLAYER_WIDTH,
                              PLAYER_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
            SDL_RenderRect(renderer, &body);
        }
    SDL_RenderPresent(renderer);
}
