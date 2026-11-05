#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "game/game.h"
#include "game/game_internal.h"
#include "game/combat/combat_internal.h"
#include "engine/engine.h"
#include "render/render.h"

static Arena arena;
static CombatState combat;
static void duel(void)
{
    arena_init(&arena);
    arena_build_default(&arena);
    combat_init(&combat);
    combat_reset_round(&combat, &arena);
    for (i32 i = 0; i < 2; i++)
    {
        combat.fighters[i].state.pos = (Vec2){100 + i * 40, 592};
        combat.fighters[i].state.grounded = true;
    }
}
static void rules(void)
{
    duel();
    PlayerCommand high = {.up_pressed = true, .up_held = true};
    fighter_apply_command(&combat.fighters[0], &high, FIXED_DT);
    assert(combat.fighters[0].state.sword_line == SWORD_LINE_HIGH &&
           !combat.fighters[0].state.throw_ready);
    duel();
    combat.fighters[0].state.is_attacking = true;
    combat_resolve_attacks(&combat);
    assert(combat.fighters[0].state.has_sword && combat.fighters[1].state.has_sword);
    assert(combat.fighters[0].state.alive && combat.fighters[1].state.alive);
    assert(combat.fighters[1].state.is_parrying);
    duel();
    combat.fighters[1].state.guard_changed = true;
    combat_resolve_attacks(&combat);
    assert(!combat.fighters[0].state.has_sword && combat.fighters[1].state.has_sword);
    assert(combat.ground_swords[0].active);
    duel();
    combat.fighters[1].state.pos.x = 140;
    combat.fighters[0].state.pos.x = 116;
    combat.fighters[0].state.action = FIGHTER_ACTION_ROLL;
    combat.fighters[0].state.vel.x = 320;
    combat.fighters[0].state.crouching = true;
    combat.fighters[1].state.sword_line = SWORD_LINE_LOW;
    combat_resolve_attacks(&combat);
    assert(!combat.fighters[0].state.alive);
    duel();
    combat.fighters[0].state.pos.x = 116;
    combat.fighters[0].state.action = FIGHTER_ACTION_ROLL;
    combat.fighters[0].state.vel.x = 320;
    combat.fighters[0].state.crouching = true;
    combat.fighters[1].state.sword_line = SWORD_LINE_HIGH;
    combat_resolve_attacks(&combat);
    assert(combat.fighters[0].state.alive);
    duel();
    combat.fighters[0].state.pos = (Vec2){116, 580};
    combat.fighters[0].state.grounded = false;
    combat.fighters[0].state.vel.y = 160;
    combat.fighters[1].state.sword_line = SWORD_LINE_HIGH;
    combat_resolve_attacks(&combat);
    assert(!combat.fighters[0].state.alive);
    duel();
    combat.fighters[0].state.grounded = false;
    PlayerCommand attack = {.thrust_pressed = true};
    fighter_apply_command(&combat.fighters[0], &attack, FIXED_DT);
    assert(combat.fighters[0].state.action == FIGHTER_ACTION_DIVE_KICK);
    assert(combat.fighters[0].state.vel.y > 0 && combat_sword_reach(&combat.fighters[0]) == 0);
    duel();
    PlayerCommand sweep = {.thrust_pressed = true, .crouch_held = true};
    fighter_apply_command(&combat.fighters[0], &sweep, FIXED_DT);
    assert(combat.fighters[0].state.action == FIGHTER_ACTION_SWEEP);
    combat.fighters[0].state.action_time = 0.08f;
    combat.fighters[1].state.pos.x = 130;
    combat_resolve_attacks(&combat);
    assert(combat.fighters[1].state.knocked_down && !combat.fighters[1].state.has_sword);
    duel();
    PlayerCommand roll = {.roll_pressed = true};
    fighter_apply_command(&combat.fighters[0], &roll, FIXED_DT);
    assert(combat.fighters[0].state.invincibility_timer == 0);
    duel();
    combat.fighters[0].state.wall_direction = 1;
    combat.fighters[0].state.grounded = false;
    PlayerCommand jump = {.jump_pressed = true};
    fighter_apply_command(&combat.fighters[0], &jump, FIXED_DT);
    assert(combat.fighters[0].state.vel.x < 0 && combat.fighters[0].state.vel.y < 0);
    duel();
    PlayerCommand throw = {.throw_pressed = true};
    combat_try_spawn_throw(&combat, 0, &throw);
    combat_try_spawn_throw(&combat, 1, &throw);
    assert(combat.thrown_sword.active && combat.second_thrown_sword.active);
    f32 initial_y = combat.thrown_sword.pos.y;
    for (i32 i = 0; i < 20; i++)
        combat_update_thrown_sword_physics(&combat, arena_get_current_room_const(&arena), FIXED_DT);
    assert(combat.thrown_sword.active && combat.thrown_sword.pos.y == initial_y);
    duel();
    PlayerInput input = {0};
    input.actions[ACTION_UP].down = true;
    input.actions[ACTION_THRUST].pressed = true;
    PlayerCommand cmd;
    game_build_player_command_from_input(&cmd, &input);
    assert(cmd.throw_pressed && !cmd.thrust_pressed);
    input = (PlayerInput){0};
    input.actions[ACTION_DOWN].pressed = true;
    input.actions[ACTION_DOWN].down = true;
    input.actions[ACTION_RIGHT].down = true;
    game_build_player_command_from_input(&cmd, &input);
    assert(cmd.roll_pressed && !cmd.jump_pressed);
}
static void themes(void)
{
    EngineConfig opts = {.title = "Nidhogg comparison", .window_width = 1280, .window_height = 720};
    Engine *engine = engine_create(&opts);
    assert(engine);
    RenderContext render = {0};
    assert(render_init(&render, engine));
    Game game = {0};
    assert(game_init(&game));
    for (i32 theme = 0; theme < 4; theme++)
    {
        ArenaOptions options = arena_options_default();
        options.theme = (ArenaTheme)theme;
        game_set_arena_options(&game, &options);
        assert(game_regenerate_arena(&game));
        game_start_match(&game);
        assert(game.arena.rooms[2].theme == theme);
        assert(combat_room_traversable(&game.arena.rooms[2]));
        render_frame(&render, engine, &game);
        SDL_Surface *surface = SDL_RenderReadPixels(engine_get_renderer_handle(engine), NULL);
        assert(surface);
        char filename[64];
        snprintf(filename, sizeof(filename), "comparison-theme-%d.bmp", theme);
        assert(SDL_SaveBMP(surface, filename));
        SDL_DestroySurface(surface);
    }
    ArenaOptions clouds = arena_options_default();
    clouds.theme = ARENA_THEME_CLOUDS;
    assert(arena_init_with_options(&game.arena, &clouds));
    game_start_match(&game);
    for (i32 i = 0; i < 2; i++)
        game.combat.fighters[i].controller.type = CONTROLLER_HUMAN;
    game.combat.fighters[0].state.pos = (Vec2){128, 592};
    game.combat.fighters[0].state.grounded = true;
    FrameInput idle = {0};
    for (i32 i = 0; i < 32; i++)
        game_update(&game, &idle, FIXED_DT);
    assert(game.arena.rooms[2].tiles[20][4] == TILE_EMPTY);
    game.phase = GAME_PHASE_VICTORY;
    game.victory_winner_index = 0;
    game.victory_time = 1.4f;
    render_frame(&render, engine, &game);
    SDL_Surface *ending = SDL_RenderReadPixels(engine_get_renderer_handle(engine), NULL);
    assert(ending);
    assert(SDL_SaveBMP(ending, "comparison-ending.bmp"));
    SDL_DestroySurface(ending);
    game_shutdown(&game);
    render_shutdown(&render);
    engine_destroy(engine);
    remove(CONFIG_FILENAME);
}
int main(void)
{
    rules();
    themes();
    puts("Nidhogg reference regression checks passed");
    return 0;
}
