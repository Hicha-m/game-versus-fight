#include <string.h>
#include <math.h>

#include "game/combat/combat.h"
#include "combat_internal.h"

static void fighter_reset_runtime(Fighter *fighter);

static f32 fighter_sword_line_world_y(const Fighter *fighter)
{
    if (!fighter)
    {
        return 0.0f;
    }

    if (fighter->state.sword_line == SWORD_LINE_HIGH)
    {
        return fighter->state.pos.y + 8.0f;
    }
    if (fighter->state.sword_line == SWORD_LINE_LOW)
    {
        return fighter->state.pos.y + PLAYER_HEIGHT - 10.0f;
    }

    return fighter->state.pos.y + PLAYER_HEIGHT * 0.5f;
}

void combat_try_spawn_throw(CombatState *combat, i32 fighter_index, const PlayerCommand *cmd)
{
    Fighter *fighter;
    ThrownSword *thrown;
    f32 spawn_x;
    f32 spawn_y;

    if (!combat || !cmd || fighter_index < 0 || fighter_index >= MAX_PLAYERS)
    {
        return;
    }

    fighter = &combat->fighters[fighter_index];
    thrown = fighter_index == 0 ? &combat->thrown_sword : &combat->second_thrown_sword;

    bool contextual_throw = cmd->thrust_pressed && fighter->state.throw_ready;
    if ((!cmd->throw_pressed && !contextual_throw) || thrown->active)
    {
        return;
    }

    if (!fighter->state.alive || !fighter->state.has_sword || fighter->state.is_stunned ||
        fighter->state.is_attacking)
    {
        return;
    }

    spawn_x = fighter->state.facing_right ? (fighter->state.pos.x + PLAYER_WIDTH + 2.0f)
                                          : (fighter->state.pos.x - 2.0f);
    spawn_y = fighter_sword_line_world_y(fighter);

    thrown->active = true;
    thrown->age = 0;
    thrown->owner_index = fighter_index;
    thrown->line = fighter->state.sword_line;
    thrown->pos.x = spawn_x;
    thrown->pos.y = spawn_y;
    thrown->vel.x =
        fighter->state.facing_right ? THROW_PROJECTILE_SPEED_X : -THROW_PROJECTILE_SPEED_X;
    thrown->vel.y = 0;
    thrown->lifetime = THROW_PROJECTILE_LIFETIME;
    thrown->rotation_deg = fighter->state.facing_right ? 0.0f : 180.0f;

    fighter->state.has_sword = false;
    fighter->state.action = FIGHTER_ACTION_THROW;
    fighter->state.action_time = 0.0f;
    fighter->state.action_duration = THROW_ACTION_DURATION;
    fighter->state.attack_cooldown = DEFAULT_ATTACK_COOLDOWN;
}

static void fighter_respawn_in_room(Fighter *fighter, const Room *room, i32 fighter_index)
{
    if (!fighter || !room)
    {
        return;
    }

    fighter_reset_runtime(fighter);
    fighter->state.invincibility_timer = 0.25f;

    if (fighter_index == 0)
    {
        fighter->state.pos = room->spawns.attacker_spawn;
        fighter->state.facing_right = true;
    }
    else
    {
        fighter->state.pos = room->spawns.defender_spawn;
        fighter->state.facing_right = false;
    }
}

static void combat_update_respawns(CombatState *combat, const Room *room, f32 dt)
{
    i32 i;

    if (!combat || !room)
    {
        return;
    }

    for (i = 0; i < MAX_PLAYERS; ++i)
    {
        Fighter *fighter = &combat->fighters[i];

        if (fighter->state.alive)
        {
            combat->respawn_timers[i] = 0.0f;
            continue;
        }

        combat->respawn_timers[i] += dt;
        if (combat->respawn_timers[i] < DEFAULT_RESPAWN_DELAY)
        {
            continue;
        }

        fighter_respawn_in_room(fighter, room, i);
        if (combat->runner_index >= 0 && combat->runner_index != i)
        {
            i32 runner = combat->runner_index;
            f32 x = combat->fighters[runner].state.pos.x + (runner == 0 ? 220.0f : -220.0f);
            x = fmaxf(32, fminf(x, room->width_tiles * TILE_SIZE - PLAYER_WIDTH - 32));
            combat_safe_spawn(room, x, &fighter->state.pos);
            fighter->state.facing_right = runner != 0;
        }
        combat->respawn_timers[i] = 0.0f;
    }
}

static void combat_update_kill_priority_timer(CombatState *combat, f32 dt)
{
    if (!combat)
    {
        return;
    }

    if (combat->kill_attacker_index >= 0 && combat->kill_attacker_index < MAX_PLAYERS)
    {
        combat->kill_attacker_timer += dt;
    }

    if (combat->kill_attacker_timer > CAMERA_KILL_PRIORITY_WINDOW ||
        (combat->kill_attacker_index >= 0 &&
         !combat->fighters[combat->kill_attacker_index].state.alive))
    {
        combat->kill_attacker_index = -1;
        combat->kill_attacker_timer = 0.0f;
    }
}

static void combat_check_camera_distance_kill(CombatState *combat, Arena *arena)
{
    i32 runner = combat->runner_index;
    const Room *room = arena_get_current_room_const(arena);
    if (!room || runner < 0 || runner >= MAX_PLAYERS)
        return;
    Fighter *leading = &combat->fighters[runner];
    Fighter *trailing = &combat->fighters[1 - runner];
    if (!leading->state.alive || !trailing->state.alive)
        return;
    f32 maximum = fmaxf(0, room->width_tiles * TILE_SIZE - WINDOW_WIDTH / 2);
    f32 left = fminf(maximum, fmaxf(0, leading->state.pos.x + PLAYER_WIDTH / 2 - WINDOW_WIDTH / 4));
    bool behind = runner == 0 ? trailing->state.pos.x + PLAYER_WIDTH < left - 64
                              : trailing->state.pos.x > left + WINDOW_WIDTH / 2 + 64;
    if (behind)
        combat_record_kill(combat, -1, 1 - runner, false);
}

static void fighter_reset_runtime(Fighter *fighter)
{
    memset(&fighter->state, 0, sizeof(fighter->state));
    fighter->state.animation_time = 0;
    fighter->state.vel.x = 0.0f;
    fighter->state.vel.y = 0.0f;
    fighter->state.grounded = true;
    fighter->state.facing_right = true;

    fighter->state.has_sword = true;
    fighter->state.sword_line = SWORD_LINE_MID;

    fighter->state.action = FIGHTER_ACTION_IDLE;
    fighter->state.action_time = 0.0f;
    fighter->state.action_duration = 0.0f;

    fighter->state.is_attacking = false;
    fighter->state.is_parrying = false;
    fighter->state.is_stunned = false;

    fighter->state.stun_timer = 0.0f;
    fighter->state.invincibility_timer = 0.0f;
    fighter->state.attack_cooldown = 0.0f;
    fighter->state.coyote_time = DEFAULT_COYOTE_TIME;
    fighter->state.move_hold_time = 0.0f;
    fighter->state.thrust_buffer_timer = 0.0f;

    fighter->state.alive = true;
}

bool combat_init(CombatState *combat)
{
    if (!combat)
    {
        return false;
    }

    memset(combat, 0, sizeof(*combat));
    combat->runner_index = -1;
    combat->kill_attacker_index = -1;
    combat->kill_victim_index = -1;
    combat->winner_index = -1;
    return true;
}

void combat_shutdown(CombatState *combat)
{
    (void)combat;
}

void combat_reset_round(CombatState *combat, const Arena *arena)
{
    const Room *room;
    Fighter *p1;
    Fighter *p2;

    if (!combat || !arena)
    {
        return;
    }

    room = arena_get_current_room_const(arena);
    if (!room)
    {
        return;
    }

    combat->runner_index = -1;
    combat->event_count = 0;
    memset(combat->ground_swords, 0, sizeof(combat->ground_swords));
    combat->round_over = false;
    combat->winner_index = -1;
    combat->kill_happened = false;
    combat->kill_attacker_index = -1;
    combat->kill_victim_index = -1;
    combat->kill_counts_for_progress = false;
    combat->kill_attacker_timer = 0.0f;
    combat->thrown_sword.active = false;
    combat->second_thrown_sword.active = false;
    combat->thrown_sword.owner_index = -1;
    combat->thrown_sword.lifetime = 0.0f;
    combat->thrown_sword.rotation_deg = 0.0f;
    combat->respawn_timers[0] = 0.0f;
    combat->respawn_timers[1] = 0.0f;

    p1 = &combat->fighters[0];
    p2 = &combat->fighters[1];

    combat->kill_happened = false;
    combat->kill_attacker_index = -1;
    combat->kill_victim_index = -1;
    combat->kill_counts_for_progress = false;

    fighter_reset_runtime(p1);
    fighter_reset_runtime(p2);

    p1->state.pos = room->spawns.attacker_spawn;
    p2->state.pos = room->spawns.defender_spawn;
    p2->state.facing_right = false;

    if (p1->stats.move_speed <= 0.0f)
    {
        p1->stats.move_speed = PLAYER_MOVE_SPEED;
        p1->stats.jump_speed = PLAYER_JUMP_SPEED;
        p1->stats.gravity = PLAYER_GRAVITY;
        p1->stats.max_fall_speed = PLAYER_MAX_FALL_SPEED;
    }

    if (p2->stats.move_speed <= 0.0f)
    {
        p2->stats.move_speed = PLAYER_MOVE_SPEED;
        p2->stats.jump_speed = PLAYER_JUMP_SPEED;
        p2->stats.gravity = PLAYER_GRAVITY;
        p2->stats.max_fall_speed = PLAYER_MAX_FALL_SPEED;
    }
}

void combat_resolve_facing(Fighter *left, Fighter *right)
{
    if (left->state.pos.x < right->state.pos.x)
    {
        left->state.facing_right = true;
        right->state.facing_right = false;
    }
    else
    {
        left->state.facing_right = false;
        right->state.facing_right = true;
    }
}

void combat_step(CombatState *combat, Arena *arena, const PlayerCommand *p1_cmd,
                 const PlayerCommand *p2_cmd, f32 dt)
{
    Room *room;
    Fighter *p1;
    Fighter *p2;

    if (!combat || !arena || !p1_cmd || !p2_cmd)
    {
        return;
    }

    if (combat->round_over)
    {
        return;
    }

    room = arena_get_current_room(arena);
    if (!room)
    {
        return;
    }

    p1 = &combat->fighters[0];
    p2 = &combat->fighters[1];

    combat->event_count = 0;
    combat->kill_happened = false;
    p1->state.is_parrying = false;
    p2->state.is_parrying = false;

    fighter_update_timers(p1, dt);
    fighter_update_timers(p2, dt);

    bool attacking_before[2] = {p1->state.is_attacking, p2->state.is_attacking};
    fighter_apply_command(p1, p1_cmd, dt);
    fighter_apply_command(p2, p2_cmd, dt);
    for (i32 i = 0; i < 2; i++)
        if (!attacking_before[i] && combat->fighters[i].state.is_attacking)
            combat_emit(combat, COMBAT_EVENT_ATTACK, i, 1 - i);
    combat_try_spawn_throw(combat, 0, p1_cmd);
    combat_try_spawn_throw(combat, 1, p2_cmd);

    fighter_apply_gravity(p1, dt);
    fighter_apply_gravity(p2, dt);

    fighter_integrate_velocity(p1, dt);
    fighter_integrate_velocity(p2, dt);

    bool alive_before[2] = {p1->state.alive, p2->state.alive};
    fighter_resolve_world_collision(p1, room, dt);
    fighter_resolve_world_collision(p2, room, dt);

    for (i32 i = 0; i < MAX_PLAYERS; i++)
    {
        GroundSword *sword = &combat->ground_swords[i];
        if (!sword->active)
            continue;
        f32 next_y = sword->pos.y + 180.0f * dt;
        if (room_get_tile(room, (i32)(sword->pos.x / TILE_SIZE),
                          (i32)((next_y + PLAYER_HEIGHT) / TILE_SIZE)) == TILE_EMPTY)
            sword->pos.y = next_y;
        for (i32 j = 0; j < MAX_PLAYERS; j++)
        {
            Fighter *f = &combat->fighters[j];
            const PlayerCommand *cmd = j == 0 ? p1_cmd : p2_cmd;
            if (f->state.alive && !f->state.has_sword && !f->state.is_stunned &&
                (cmd->down_pressed || cmd->crouch_held || f->state.action == FIGHTER_ACTION_ROLL) &&
                fabsf(f->state.pos.x - sword->pos.x) < 32 &&
                fabsf(f->state.pos.y - sword->pos.y) < 48)
            {
                f->state.has_sword = true;
                sword->active = false;
                break;
            }
        }
    }
    combat_update_thrown_sword_physics(combat, room, dt);
    combat_resolve_thrown_sword_hits(combat);
    combat_resolve_attacks(combat);
    combat_check_fall_deaths(combat, room);
    for (i32 i = 0; i < MAX_PLAYERS; ++i)
        if (alive_before[i] && !combat->fighters[i].state.alive)
        {
            bool recorded = false;
            for (i32 j = 0; j < combat->event_count; j++)
                if (combat->events[j].type == COMBAT_EVENT_DEATH && combat->events[j].target == i)
                    recorded = true;
            if (!recorded)
                combat_record_kill(combat, -1, i, false);
        }
    combat_update_respawns(combat, room, dt);
    /* Facing is controlled by movement, including during a chase. */

    combat_update_kill_priority_timer(combat, dt);
    combat_check_camera_distance_kill(combat, arena);
    fighter_sync_action(p1);
    fighter_sync_action(p2);
}
