#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "game/game.h"
#include "game/game_internal.h"
#include "game/combat/combat_internal.h"
#include "game/ai/ai_internal.h"
static Arena a;
static CombatState c;
static void setup(void)
{
    ArenaOptions o = arena_options_default();
    assert(arena_init_with_options(&a, &o));
    assert(combat_init(&c));
    combat_reset_round(&c, &a);
    for (int i = 0; i < 2; i++)
    {
        c.fighters[i].state.pos = (Vec2){100 + 40 * i, 400};
        c.fighters[i].state.grounded = true;
    }
}
int main(void)
{
    setup();
    PlayerCommand cmd = ai_action_to_command(AI_ACTION_LINE_HIGH);
    fighter_apply_command(&c.fighters[0], &cmd, FIXED_DT);
    assert(c.fighters[0].state.sword_line == SWORD_LINE_HIGH);
    setup();
    c.fighters[0].state.is_attacking = true;
    c.fighters[1].state.facing_right = true;
    combat_resolve_attacks(&c);
    assert(!c.fighters[1].state.alive);
    assert(c.fighters[0].state.has_sword);
    setup();
    for (int i = 0; i < 2; i++)
        c.fighters[i].state.is_attacking = true;
    c.fighters[0].state.sword_line = SWORD_LINE_HIGH;
    c.fighters[1].state.sword_line = SWORD_LINE_LOW;
    combat_resolve_attacks(&c);
    assert(!c.fighters[0].state.alive && !c.fighters[1].state.alive);
    assert(!c.kill_counts_for_progress);
    setup();
    c.fighters[0].state.is_stunned = true;
    c.fighters[0].state.vel.x = -400;
    cmd = (PlayerCommand){0};
    fighter_apply_command(&c.fighters[0], &cmd, FIXED_DT);
    assert(c.fighters[0].state.vel.x < -300);
    setup();
    c.fighters[0].state.grounded = false;
    cmd.thrust_pressed = true;
    fighter_apply_command(&c.fighters[0], &cmd, FIXED_DT);
    assert(c.fighters[0].state.is_attacking);
    setup();
    Room room = {0};
    room.width_tiles = 40;
    room.height_tiles = 22;
    room.tiles[10][4] = TILE_SOLID;
    c.fighters[0].state.pos = (Vec2){103, 320};
    c.fighters[0].state.vel = (Vec2){320, 0};
    fighter_integrate_velocity(&c.fighters[0], FIXED_DT);
    fighter_resolve_world_collision(&c.fighters[0], &room, FIXED_DT);
    assert(c.fighters[0].state.pos.x + PLAYER_WIDTH <= 128);
    assert(c.fighters[0].state.pos.y == 320);
    setup();
    c.fighters[0].state.is_attacking = true;
    c.fighters[1].state.guard_changed = true;
    combat_resolve_attacks(&c);
    assert(!c.fighters[0].state.has_sword);
    assert(c.ground_swords[0].active);
    c.fighters[0].state.is_stunned = false;
    c.fighters[0].state.stun_timer = 0;
    c.fighters[1].state.pos.x = 1000;
    cmd = (PlayerCommand){.down_pressed = true};
    PlayerCommand idle = {0};
    combat_step(&c, &a, &cmd, &idle, FIXED_DT);
    assert(c.fighters[0].state.has_sword);
    Game g = {0};
    FrameInput input = {0};
    assert(game_init(&g));
    game_start_match(&g);
    g.combat.fighters[0].controller.type = CONTROLLER_HUMAN;
    g.combat.fighters[1].controller.type = CONTROLLER_HUMAN;
    g.room_push_direction = 1;
    g.combat.runner_index = 0;
    const Room *r = arena_get_current_room_const(&g.arena);
    g.combat.fighters[0].state.pos = (Vec2){r->width_tiles * TILE_SIZE - PLAYER_WIDTH, 400};
    g.combat.fighters[0].state.alive = false;
    game_update(&g, &input, FIXED_DT);
    assert(g.arena.current_room == 2);
    g.combat.fighters[0].state.alive = true;
    g.combat.fighters[0].state.has_sword = false;
    g.room_push_direction = 1;
    game_handle_room_transitions(&g);
    assert(g.arena.current_room == 3);
    assert(!g.combat.fighters[0].state.has_sword);
    assert(!g.combat.fighters[1].state.alive);
    g.match_stats.kills[0] = 99;
    game_start_match(&g);
    assert(g.match_stats.kills[0] == 0);
    game_shutdown(&g);
    GameConfig before, after;
    config_set_defaults(&before);
    before.keybinds[0].keys[0] = 11;
    before.keybinds[1].keys[0] = 22;
    assert(config_save(&before));
    assert(config_load(&after));
    assert(after.keybinds[0].keys[0] == 11 && after.keybinds[1].keys[0] == 22);
    FILE *f = fopen(CONFIG_FILENAME, "w");
    assert(f);
    fputs("      \"key_-1\": 55\n", f);
    fclose(f);
    assert(!config_load(&after));
    remove(CONFIG_FILENAME);
    puts("Regression scenarios passed");
    return 0;
}
