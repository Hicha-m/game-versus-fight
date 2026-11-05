#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "game/game.h"
#include "game/game_internal.h"
#include "game/combat/combat_internal.h"
#include "engine/engine_internal.h"
#include "render/render.h"
#include "render/assets_private.h"

static void menu_and_config(void)
{
    MenuContext menu = {0}, other = {0};
    assert(menu_init(&menu));
    assert(menu_init(&other));
    menu_set_state(&menu, MENU_STATE_OPTIONS_KEYBINDS);
    FrameInput in = {0};
    in.players[0].actions[ACTION_THRUST].pressed = true;
    menu_update(&menu, &in, FIXED_DT);
    assert(menu.capturing_key);
    in = (FrameInput){0};
    in.key_pressed = 9;
    menu_update(&menu, &in, FIXED_DT);
    assert(!menu.capturing_key && menu.config.keybinds[0].keys[0] == 9);
    in = (FrameInput){0};
    in.players[0].actions[ACTION_RIGHT].pressed = true;
    menu_update(&menu, &in, FIXED_DT);
    assert(menu.keybind_player == 1);
    menu.capturing_key = true;
    in = (FrameInput){0};
    in.key_pressed = 10;
    menu_update(&menu, &in, FIXED_DT);
    assert(menu.config.keybinds[1].keys[0] == 10);
    GameConfig loaded;
    assert(config_load(&loaded));
    assert(loaded.keybinds[0].keys[0] == 9 && loaded.keybinds[1].keys[0] == 10);
    menu.capturing_key = true;
    in = (FrameInput){0};
    in.pause.pressed = true;
    menu_update(&menu, &in, FIXED_DT);
    assert(!menu.capturing_key);
    menu.pending_action = MENU_ACTION_QUIT;
    assert(menu_get_action(&other) == MENU_ACTION_NONE);
    assert(menu_get_action(&menu) == MENU_ACTION_QUIT);
    const char *invalid[] = {"{", "{\"audio\":{\"master_volume\":nan}}",
                             "{\"player1\":{\"key_-1\":8}}", "{\"player1\":{\"key_0\":9999}}",
                             "{\"audio\":{\"master_volume\":0.5,}}"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); i++)
    {
        FILE *f = fopen(CONFIG_FILENAME, "w");
        assert(f);
        fputs(invalid[i], f);
        fclose(f);
        assert(!config_load(&loaded));
    }
    FILE *f = fopen(CONFIG_FILENAME, "w");
    assert(f);
    fputs("{\"audio\":{\"master_volume\":0.25},\"gameplay\":{\"fullscreen\":true},\"keybinds\":{"
          "\"player1\":{\"key_0\":11},\"player2\":{\"key_0\":12}}}",
          f);
    fclose(f);
    assert(config_load(&loaded));
    assert(loaded.master_volume == 0.25f && loaded.fullscreen && loaded.keybinds[1].keys[0] == 12);
    remove(CONFIG_FILENAME);
}

static void terrain(void)
{
    Game game = {0};
    assert(game_init(&game));
    for (i32 difficulty = 0; difficulty <= 10; difficulty++)
        for (u32 seed = 0; seed < 12; seed++)
        {
            ArenaOptions options = arena_options_procedural(seed, difficulty);
            game_set_arena_options(&game, &options);
            assert(game_regenerate_arena(&game));
            Arena first = game.arena;
            assert(game_regenerate_arena(&game));
            assert(memcmp(&first, &game.arena, sizeof(first)) == 0);
            for (i32 i = 0; i < game.arena.room_count; i++)
                assert(combat_room_traversable(&game.arena.rooms[i]));
        }
    Room bad = {0};
    bad.width_tiles = 40;
    bad.height_tiles = 22;
    bad.spawns.attacker_spawn = (Vec2){128, 500};
    bad.spawns.defender_spawn = (Vec2){1000, 500};
    bad.transition.right_trigger_x = 1268;
    bad.transition.left_trigger_x = 12;
    assert(!combat_room_traversable(&bad));
    game_shutdown(&game);
    remove(CONFIG_FILENAME);
}

static void combat_and_ai(void)
{
    Game g = {0};
    assert(game_init(&g));
    game_start_match(&g);
    for (i32 i = 0; i < 2; i++)
    {
        g.combat.fighters[i].controller.type = CONTROLLER_HUMAN;
        g.combat.fighters[i].state.pos = (Vec2){100 + i * 40, 592};
        g.combat.fighters[i].state.grounded = true;
    }
    g.combat.fighters[0].state.sword_line = SWORD_LINE_HIGH;
    g.combat.fighters[1].state.sword_line = SWORD_LINE_LOW;
    FrameInput in = {0};
    in.players[0].actions[ACTION_THRUST].pressed = true;
    in.players[1].actions[ACTION_THRUST].pressed = true;
    for (i32 tick = 0; tick < 8; tick++)
    {
        game_update(&g, &in, FIXED_DT);
        input_clear_edges(&in);
    }
    assert(!g.combat.fighters[0].state.alive && !g.combat.fighters[1].state.alive);
    assert(g.match_stats.kills[0] == 1 && g.match_stats.kills[1] == 1 &&
           g.room_push_direction == 0);
    game_start_match(&g);
    assert(g.match_stats.kills[0] == 0);
    g.combat.fighters[0].state.grounded = true;
    PlayerCommand roll = {.roll_pressed = true};
    fighter_apply_command(&g.combat.fighters[0], &roll, FIXED_DT);
    assert(g.combat.fighters[0].state.action == FIGHTER_ACTION_ROLL);
    PlayerCommand idle = {0};
    fighter_apply_command(&g.combat.fighters[0], &idle, FIXED_DT);
    assert(g.combat.fighters[0].state.action == FIGHTER_ACTION_ROLL);
    for (i32 algorithm = AI_ALGO_SCRIPTED; algorithm <= AI_ALGO_MINIMAX_ALPHA_BETA; algorithm++)
    {
        AIController ai;
        ai_init(&ai);
        ai.search_depth = 2;
        ai_set_algorithm(&ai, (AIAlgorithm)algorithm);
        PlayerCommand command =
            ai_think(&ai, &g.arena, &g.combat, 1, FIXED_DT, g.match_stats.kills);
        assert(!(command.left_pressed && command.right_pressed));
        assert(ai.metrics.nodes_expanded > 0 && ai.metrics.nodes_expanded <= 1032);
        if (algorithm == AI_ALGO_MINIMAX)
            assert(ai.metrics.cutoffs == 0);
        if (algorithm != AI_ALGO_SCRIPTED)
            assert(ai.metrics.estimated_memory_bytes > 0);
    }
    AIController minimax, alpha;
    ai_init(&minimax);
    ai_init(&alpha);
    minimax.search_depth = alpha.search_depth = 2;
    ai_set_algorithm(&minimax, AI_ALGO_MINIMAX);
    ai_set_algorithm(&alpha, AI_ALGO_MINIMAX_ALPHA_BETA);
    PlayerCommand m = ai_think(&minimax, &g.arena, &g.combat, 1, FIXED_DT, NULL);
    PlayerCommand b = ai_think(&alpha, &g.arena, &g.combat, 1, FIXED_DT, NULL);
    assert(m.right_pressed == b.right_pressed && m.left_pressed == b.left_pressed &&
           m.target_sword_line == b.target_sword_line && m.thrust_pressed == b.thrust_pressed);
    assert(alpha.metrics.nodes_expanded <= minimax.metrics.nodes_expanded);
    g.combat.runner_index = 0;
    g.room_push_direction = 1;
    g.arena.current_room = ROOM_COUNT - 1;
    const Room *room = arena_get_current_room_const(&g.arena);
    g.combat.fighters[0].state.pos = (Vec2){room->width_tiles * TILE_SIZE - PLAYER_WIDTH, 592};
    g.combat.fighters[0].state.action = FIGHTER_ACTION_IDLE;
    g.combat.fighters[0].state.action_duration = 0;
    game_update(&g, &in, FIXED_DT);
    assert(g.phase == GAME_PHASE_VICTORY && g.victory_winner_index == 0);
    game_shutdown(&g);
    remove(CONFIG_FILENAME);
}

static void physics_and_unarmed(void)
{
    Room room = {0};
    room.width_tiles = 40;
    room.height_tiles = 22;
    room.tiles[7][4] = TILE_SOLID;
    Fighter f = {0};
    f.state.alive = true;
    f.state.pos = (Vec2){128, 280};
    f.state.vel.y = -600;
    f.stats =
        (FighterStats){PLAYER_MOVE_SPEED, PLAYER_JUMP_SPEED, PLAYER_GRAVITY, PLAYER_MAX_FALL_SPEED};
    for (i32 i = 0; i < 4; i++)
    {
        fighter_apply_gravity(&f, FIXED_DT);
        fighter_integrate_velocity(&f, FIXED_DT);
        fighter_resolve_world_collision(&f, &room, FIXED_DT);
    }
    assert(f.state.pos.y >= 256 && f.state.vel.y >= 0);
    memset(room.tiles, 0, sizeof(room.tiles));
    room.tiles[10][4] = TILE_PLATFORM;
    f.state.pos = (Vec2){128, 340};
    f.state.vel.y = -620;
    f.state.grounded = false;
    bool above = false, landed = false;
    for (i32 i = 0; i < 80; i++)
    {
        fighter_apply_gravity(&f, FIXED_DT);
        fighter_integrate_velocity(&f, FIXED_DT);
        fighter_resolve_world_collision(&f, &room, FIXED_DT);
        if (f.state.pos.y + PLAYER_HEIGHT < 320)
            above = true;
        if (above && f.state.grounded)
        {
            landed = true;
            break;
        }
    }
    assert(above && landed && fabsf(f.state.pos.y - 272) < 0.01f);
    room.tiles[10][4] = TILE_HAZARD;
    f.state.pos = (Vec2){128, 320};
    f.state.vel = (Vec2){0, 0};
    fighter_resolve_world_collision(&f, &room, FIXED_DT);
    assert(!f.state.alive);
    Arena arena;
    arena_init(&arena);
    arena_build_default(&arena);
    CombatState combat;
    combat_init(&combat);
    combat_reset_round(&combat, &arena);
    for (i32 i = 0; i < 2; i++)
    {
        combat.fighters[i].state.pos = (Vec2){100 + i * 30, 400};
        combat.fighters[i].state.has_sword = false;
    }
    combat.fighters[0].state.is_attacking = true;
    combat_resolve_attacks(&combat);
    assert(combat.fighters[1].state.alive && combat.fighters[1].state.is_stunned);
    combat.fighters[0].state.is_attacking = true;
    combat_resolve_attacks(&combat);
    assert(combat.fighters[1].state.alive && combat.fighters[1].state.knocked_down);
    combat.fighters[0].state.crouching = true;
    combat.fighters[0].state.is_attacking = true;
    combat_resolve_attacks(&combat);
    assert(!combat.fighters[1].state.alive);
    combat_reset_round(&combat, &arena);
    PlayerCommand roll = {.roll_pressed = true};
    combat.fighters[0].state.grounded = true;
    fighter_apply_command(&combat.fighters[0], &roll, FIXED_DT);
    for (i32 i = 0; i < 20; i++)
        fighter_update_timers(&combat.fighters[0], FIXED_DT);
    fighter_sync_action(&combat.fighters[0]);
    assert(combat.fighters[0].state.action != FIGHTER_ACTION_ROLL);
}

static void render_audio(void)
{
    EngineConfig config = {
        .title = "Audit integration", .window_width = 1280, .window_height = 720};
    Engine *engine = engine_create(&config);
    assert(engine);
    assert(engine->audio);
    SDL_PauseAudioStreamDevice(engine->audio);
    SDL_ClearAudioStream(engine->audio);
    engine_audio_update(engine, 0, 1, 1, true, 1);
    assert(SDL_GetAudioStreamQueued(engine->audio) > 0);
    float samples[600];
    int read = SDL_GetAudioStreamData(engine->audio, samples, sizeof(samples));
    assert(read > 0);
    for (i32 i = 0; i < read / (i32)sizeof(float); i++)
        assert(samples[i] == 0);
    SDL_ClearAudioStream(engine->audio);
    engine_audio_update(engine, 1, 1, 1, true, 2);
    assert(SDL_GetAudioStreamQueued(engine->audio) > 0);
    read = SDL_GetAudioStreamData(engine->audio, samples, sizeof(samples));
    assert(read > 0);
    bool audible = false;
    for (i32 i = 0; i < read / (i32)sizeof(float); i++)
        if (fabsf(samples[i]) > 0.001f)
            audible = true;
    assert(audible);
    RenderContext render = {0};
    assert(render_init(&render, engine));
    assert(assets_animation(0, 0, true));
    assert(assets_animation(0, 0.1f, true) != assets_animation(0, 0, true));
    Game game = {0};
    assert(game_init(&game));
    game_start_match(&game);
    for (i32 action = FIGHTER_ACTION_IDLE; action <= FIGHTER_ACTION_DEAD; action++)
    {
        game.combat.fighters[0].state.action = (FighterAction)action;
        game.combat.fighters[0].state.animation_time = 0.1f;
        render_frame(&render, engine, &game);
    }
    game.combat.fighters[0].state.action = FIGHTER_ACTION_RUN;
    game.combat.fighters[0].state.alive = true;
    render_frame(&render, engine, &game);
    for (i32 i = 0; i < 2; i++)
        assert(game.combat.fighters[i].state.pos.x >= render.camera.position.x &&
               game.combat.fighters[i].state.pos.x < render.camera.position.x + WINDOW_WIDTH);
    SDL_Surface *pixels = SDL_RenderReadPixels(engine->renderer, NULL);
    assert(pixels);
    assert(SDL_SaveBMP(pixels, "audit-frame.bmp"));
    SDL_DestroySurface(pixels);
    game.phase = GAME_PHASE_MENU;
    menu_set_state(&game.menu, MENU_STATE_OPTIONS_KEYBINDS);
    render_frame(&render, engine, &game);
    u16 keys[2][7];
    for (i32 p = 0; p < 2; p++)
        for (i32 a = 0; a < 7; a++)
            keys[p][a] = game.menu.config.keybinds[p].keys[a];
    keys[0][0] = 9;
    engine_set_keys(engine, (const u16(*)[7])keys);
    assert(engine->keybinds[0].right == 9);
    game_shutdown(&game);
    render_shutdown(&render);
    engine_destroy(engine);
    remove(CONFIG_FILENAME);
}
int main(void)
{
    physics_and_unarmed();
    menu_and_config();
    combat_and_ai();
    terrain();
    render_audio();
    puts("Audit acceptance checks passed");
    return 0;
}
