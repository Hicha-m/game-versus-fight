#include <string.h>
#include <math.h>

#include "game/combat/combat.h"
#include "combat_internal.h"

static void fighter_reset_runtime(Fighter* fighter);

static f32 fighter_sword_line_world_y(const Fighter* fighter)
{
    if (!fighter) {
        return 0.0f;
    }

    if (fighter->state.sword_line == SWORD_LINE_HIGH) {
        return fighter->state.pos.y + 8.0f;
    }
    if (fighter->state.sword_line == SWORD_LINE_LOW) {
        return fighter->state.pos.y + PLAYER_HEIGHT - 10.0f;
    }

    return fighter->state.pos.y + PLAYER_HEIGHT * 0.5f;
}

void combat_try_spawn_throw(CombatState* combat, i32 fighter_index, const PlayerCommand* cmd)
{
    Fighter* fighter;
    ThrownSword* thrown;
    f32 spawn_x;
    f32 spawn_y;

    if (!combat || !cmd || fighter_index < 0 || fighter_index >= MAX_PLAYERS) {
        return;
    }

    fighter = &combat->fighters[fighter_index];
    thrown = &combat->thrown_sword;

    if (!cmd->throw_pressed || thrown->active) {
        return;
    }

    if (!fighter->state.alive || !fighter->state.has_sword || fighter->state.is_stunned || fighter->state.is_attacking) {
        return;
    }

    spawn_x = fighter->state.facing_right
        ? (fighter->state.pos.x + PLAYER_WIDTH + 2.0f)
        : (fighter->state.pos.x - 2.0f);
    spawn_y = fighter_sword_line_world_y(fighter);

    thrown->active = true;
    thrown->owner_index = fighter_index;
    thrown->line = fighter->state.sword_line;
    thrown->pos.x = spawn_x;
    thrown->pos.y = spawn_y;
    thrown->vel.x = fighter->state.facing_right ? THROW_PROJECTILE_SPEED_X : -THROW_PROJECTILE_SPEED_X;
    if (fighter->state.sword_line == SWORD_LINE_HIGH) {
        thrown->vel.y = THROW_PROJECTILE_SPEED_Y_HIGH;
    } else if (fighter->state.sword_line == SWORD_LINE_LOW) {
        thrown->vel.y = THROW_PROJECTILE_SPEED_Y_LOW;
    } else {
        thrown->vel.y = THROW_PROJECTILE_SPEED_Y_MID;
    }
    thrown->lifetime = THROW_PROJECTILE_LIFETIME;
    thrown->rotation_deg = fighter->state.facing_right ? 0.0f : 180.0f;

    fighter->state.has_sword = false;
    fighter->state.action = FIGHTER_ACTION_THROW;
    fighter->state.action_time = 0.0f;
    fighter->state.action_duration = THROW_ACTION_DURATION;
    fighter->state.attack_cooldown = DEFAULT_ATTACK_COOLDOWN;
}

static void fighter_respawn_in_room(Fighter* fighter, const Room* room, i32 fighter_index)
{
    if (!fighter || !room) {
        return;
    }

    fighter_reset_runtime(fighter);

    if (fighter_index == 0) {
        fighter->state.pos = room->spawns.attacker_spawn;
        fighter->state.facing_right = true;
    } else {
        fighter->state.pos = room->spawns.defender_spawn;
        fighter->state.facing_right = false;
    }
}

static void combat_update_respawns(CombatState* combat, const Room* room, f32 dt)
{
    i32 i;

    if (!combat || !room) {
        return;
    }

    for (i = 0; i < MAX_PLAYERS; ++i) {
        Fighter* fighter = &combat->fighters[i];

        if (fighter->state.alive) {
            combat->respawn_timers[i] = 0.0f;
            continue;
        }

        combat->respawn_timers[i] += dt;
        if (combat->respawn_timers[i] < DEFAULT_RESPAWN_DELAY) {
            continue;
        }

        fighter_respawn_in_room(fighter, room, i);
        combat->respawn_timers[i] = 0.0f;
    }
}

static void combat_update_kill_priority_timer(CombatState* combat, f32 dt)
{
    if (!combat) {
        return;
    }

    if (combat->kill_attacker_index >= 0 && combat->kill_attacker_index < MAX_PLAYERS) {
        combat->kill_attacker_timer += dt;
    }

    if (combat->kill_attacker_timer > CAMERA_KILL_PRIORITY_WINDOW ||
        (combat->kill_attacker_index >= 0 &&
         !combat->fighters[combat->kill_attacker_index].state.alive)) {
        combat->kill_attacker_index = -1;
        combat->kill_attacker_timer = 0.0f;
    }
}

static void combat_check_camera_distance_kill(CombatState* combat, Arena* arena)
{
    const Room* room;
    f32 camera_center_x;
    Fighter* killer;
    Fighter* victim;
    i32 killer_idx;
    i32 victim_idx;
    f32 victim_distance_from_camera;
    f32 killer_x;
    f32 victim_x;

    if (!combat || !arena ||
        combat->kill_attacker_index < 0 ||
        combat->kill_attacker_index >= MAX_PLAYERS) {
        return;
    }

    room = arena_get_current_room_const(arena);
    if (!room) {
        return;
    }

    killer_idx = combat->kill_attacker_index;
    victim_idx = 1 - killer_idx;
    killer = &combat->fighters[killer_idx];
    victim = &combat->fighters[victim_idx];

    if (!killer->state.alive || !victim->state.alive) {
        return;
    }

    killer_x = killer->state.pos.x + PLAYER_WIDTH * 0.5f;
    camera_center_x = killer_x;

    victim_x = victim->state.pos.x + PLAYER_WIDTH * 0.5f;
    victim_distance_from_camera = fabsf(victim_x - camera_center_x);

    if (victim_distance_from_camera > CAMERA_MAX_DISTANCE) {
        victim->state.alive = false;
        combat->kill_happened = true;
        combat->kill_attacker_index = killer_idx;
        combat->kill_victim_index = victim_idx;
        combat->kill_counts_for_progress = true;
        combat->respawn_timers[victim_idx] = 0.0f;
    }
}

static void fighter_reset_runtime(Fighter* fighter)
{
    fighter->state.vel.x = 0.0f;
    fighter->state.vel.y = 0.0f;
    fighter->state.grounded = false;
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

bool combat_init(CombatState* combat)
{
    if (!combat) {
        return false;
    }

    memset(combat, 0, sizeof(*combat));
    combat->kill_attacker_index = -1;
    combat->kill_victim_index = -1;
    combat->winner_index = -1;
    return true;
}

void combat_shutdown(CombatState* combat)
{
    (void)combat;
}

void combat_reset_round(CombatState* combat, const Arena* arena)
{
    const Room* room;
    Fighter* p1;
    Fighter* p2;

    if (!combat || !arena) {
        return;
    }

    room = arena_get_current_room_const(arena);
    if (!room) {
        return;
    }

    combat->round_over = false;
    combat->winner_index = -1;
    combat->kill_happened = false;
    combat->kill_attacker_index = -1;
    combat->kill_victim_index = -1;
    combat->kill_counts_for_progress = false;
    combat->kill_attacker_timer = 0.0f;
    combat->thrown_sword.active = false;
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

    if (p1->stats.move_speed <= 0.0f) {
        p1->stats.move_speed = PLAYER_MOVE_SPEED;
        p1->stats.jump_speed = PLAYER_JUMP_SPEED;
        p1->stats.gravity = PLAYER_GRAVITY;
        p1->stats.max_fall_speed = PLAYER_MAX_FALL_SPEED;
    }

    if (p2->stats.move_speed <= 0.0f) {
        p2->stats.move_speed = PLAYER_MOVE_SPEED;
        p2->stats.jump_speed = PLAYER_JUMP_SPEED;
        p2->stats.gravity = PLAYER_GRAVITY;
        p2->stats.max_fall_speed = PLAYER_MAX_FALL_SPEED;
    }
}

void combat_resolve_facing(Fighter* left, Fighter* right)
{
    if (left->state.pos.x < right->state.pos.x) {
        left->state.facing_right = true;
        right->state.facing_right = false;
    } else {
        left->state.facing_right = false;
        right->state.facing_right = true;
    }
}

void combat_step(
    CombatState* combat,
    Arena* arena,
    const PlayerCommand* p1_cmd,
    const PlayerCommand* p2_cmd,
    f32 dt
)
{
    Room* room;
    Fighter* p1;
    Fighter* p2;

    if (!combat || !arena || !p1_cmd || !p2_cmd) {
        return;
    }

    if (combat->round_over) {
        return;
    }

    room = arena_get_current_room(arena);
    if (!room) {
        return;
    }

    p1 = &combat->fighters[0];
    p2 = &combat->fighters[1];

    p1->state.is_parrying = false;
    p2->state.is_parrying = false;

    fighter_update_timers(p1, dt);
    fighter_update_timers(p2, dt);

    fighter_apply_command(p1, p1_cmd, dt);
    fighter_apply_command(p2, p2_cmd, dt);
    combat_try_spawn_throw(combat, 0, p1_cmd);
    combat_try_spawn_throw(combat, 1, p2_cmd);

    fighter_apply_gravity(p1, dt);
    fighter_apply_gravity(p2, dt);

    fighter_integrate_velocity(p1, dt);
    fighter_integrate_velocity(p2, dt);

    fighter_resolve_world_collision(p1, room, dt);
    fighter_resolve_world_collision(p2, room, dt);

    combat_update_thrown_sword_physics(combat, room, dt);
    combat_resolve_thrown_sword_hits(combat);
    combat_resolve_attacks(combat);
    combat_check_fall_deaths(combat, room);
    combat_update_respawns(combat, room, dt);
    combat_resolve_facing(p1, p2);

    combat_update_kill_priority_timer(combat, dt);
    combat_check_camera_distance_kill(combat, arena);
}
