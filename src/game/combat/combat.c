#include <math.h>
#include <string.h>

#include "game/combat/combat.h"
#include "combat_internal.h"

static f32 clampf(f32 v, f32 lo, f32 hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static bool room_ground_contact_at(
    const Room* room,
    f32 foot_x,
    f32 feet_y,
    f32 prev_feet_y,
    bool falling,
    f32* out_snap_y
)
{
    i32 tx;
    i32 ty;
    TileType tile;
    f32 tile_top;

    if (!room || !out_snap_y) {
        return false;
    }

    tx = (i32)(foot_x / TILE_SIZE);
    ty = (i32)(feet_y / TILE_SIZE);
    tile = room_get_tile(room, tx, ty);
    tile_top = (f32)(ty * TILE_SIZE);

    if (tile == TILE_SOLID) {
        *out_snap_y = tile_top - PLAYER_HEIGHT;
        return true;
    }

    if (tile == TILE_PLATFORM && falling && prev_feet_y <= tile_top + 0.01f) {
        *out_snap_y = tile_top - PLAYER_HEIGHT;
        return true;
    }

    return false;
}

static FighterAction sword_line_to_attack_action(SwordLine line)
{
    switch (line) {
        case SWORD_LINE_HIGH: return FIGHTER_ACTION_ATTACK_HIGH;
        case SWORD_LINE_LOW: return FIGHTER_ACTION_ATTACK_LOW;
        case SWORD_LINE_MID:
        default: return FIGHTER_ACTION_ATTACK_MID;
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

    fighter->state.alive = true;
}

RectF fighter_get_body_rect(const Fighter* fighter)
{
    RectF r;
    r.x = fighter->state.pos.x;
    r.y = fighter->state.pos.y;
    r.w = PLAYER_WIDTH;
    r.h = PLAYER_HEIGHT;
    return r;
}

bool fighters_in_attack_range(const Fighter* attacker, const Fighter* defender, bool armed)
{
    f32 dx;
    f32 dy;
    f32 x_range;
    bool facing_ok;

    dx = defender->state.pos.x - attacker->state.pos.x;
    dy = fabsf(defender->state.pos.y - attacker->state.pos.y);

    x_range = armed ? SWORD_THRUST_RANGE_X : (SWORD_THRUST_RANGE_X * 0.6f);
    facing_ok = false;

    if (attacker->state.facing_right && dx >= 0.0f && dx <= x_range) {
        facing_ok = true;
    } else if (!attacker->state.facing_right && dx <= 0.0f && fabsf(dx) <= x_range) {
        facing_ok = true;
    }

    return facing_ok && (dy <= (armed ? SWORD_HIT_RANGE_Y : (SWORD_HIT_RANGE_Y * 1.2f)));
}

bool combat_init(CombatState* combat)
{
    if (!combat) {
        return false;
    }

    memset(combat, 0, sizeof(*combat));
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

    p1 = &combat->fighters[0];
    p2 = &combat->fighters[1];

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

void fighter_update_timers(Fighter* fighter, f32 dt)
{
    if (!fighter->state.alive) {
        return;
    }

    if (fighter->state.attack_cooldown > 0.0f) {
        fighter->state.attack_cooldown -= dt;
        if (fighter->state.attack_cooldown < 0.0f) {
            fighter->state.attack_cooldown = 0.0f;
        }
    }

    if (fighter->state.stun_timer > 0.0f) {
        fighter->state.stun_timer -= dt;
        if (fighter->state.stun_timer <= 0.0f) {
            fighter->state.stun_timer = 0.0f;
            fighter->state.is_stunned = false;
            fighter->state.action = FIGHTER_ACTION_IDLE;
        }
    }

    if (fighter->state.invincibility_timer > 0.0f) {
        fighter->state.invincibility_timer -= dt;
        if (fighter->state.invincibility_timer < 0.0f) {
            fighter->state.invincibility_timer = 0.0f;
        }
    }

    if (fighter->state.action_duration > 0.0f) {
        fighter->state.action_time += dt;
        if (fighter->state.action_time >= fighter->state.action_duration) {
            fighter->state.action_time = 0.0f;
            fighter->state.action_duration = 0.0f;
            fighter->state.is_attacking = false;
            if (fighter->state.grounded && !fighter->state.is_stunned) {
                fighter->state.action = FIGHTER_ACTION_IDLE;
            }
        }
    }

    if (!fighter->state.grounded) {
        fighter->state.coyote_time -= dt;
        if (fighter->state.coyote_time < 0.0f) {
            fighter->state.coyote_time = 0.0f;
        }
    }
}

void fighter_apply_command(Fighter* fighter, const PlayerCommand* cmd, f32 dt)
{
    (void)dt;

    if (!fighter || !cmd || !fighter->state.alive) {
        return;
    }

    if (fighter->state.is_stunned) {
        fighter->state.vel.x = 0.0f;
        return;
    }

    fighter->state.sword_line = cmd->target_sword_line;
    fighter->state.vel.x = (f32)cmd->move_x * fighter->stats.move_speed;

    if (cmd->move_x != 0) {
        fighter->state.facing_right = (cmd->move_x > 0);
        if (fighter->state.grounded && !fighter->state.is_attacking && fighter->state.action != FIGHTER_ACTION_ROLL) {
            fighter->state.action = FIGHTER_ACTION_RUN;
        }
    } else if (fighter->state.grounded && !fighter->state.is_attacking && fighter->state.action != FIGHTER_ACTION_ROLL) {
        fighter->state.action = FIGHTER_ACTION_IDLE;
    }

    if ((fighter->state.grounded || fighter->state.coyote_time > 0.0f) && cmd->jump_pressed) {
        fighter->state.vel.y = fighter->stats.jump_speed;
        fighter->state.grounded = false;
        fighter->state.coyote_time = 0.0f;
        fighter->state.action = FIGHTER_ACTION_JUMP;
    }

    if (cmd->roll_pressed && fighter->state.grounded && !fighter->state.is_attacking) {
        fighter->state.action = FIGHTER_ACTION_ROLL;
        fighter->state.action_duration = DEFAULT_ROLL_DURATION;
        fighter->state.action_time = 0.0f;
    }

    if (cmd->throw_pressed && fighter->state.has_sword) {
        fighter->state.has_sword = false;
        fighter->state.attack_cooldown = DEFAULT_ATTACK_COOLDOWN;
    }

    if (cmd->thrust_pressed && fighter->state.attack_cooldown <= 0.0f) {
        fighter->state.is_attacking = true;
        fighter->state.action = sword_line_to_attack_action(fighter->state.sword_line);
        fighter->state.action_time = 0.0f;
        fighter->state.action_duration = DEFAULT_ATTACK_DURATION;
        fighter->state.attack_cooldown = DEFAULT_ATTACK_COOLDOWN;
    }
}

void fighter_apply_gravity(Fighter* fighter, f32 dt)
{
    if (!fighter->state.alive) {
        return;
    }

    if (!fighter->state.grounded) {
        fighter->state.vel.y += fighter->stats.gravity * dt;
        fighter->state.vel.y = clampf(fighter->state.vel.y, -10000.0f, fighter->stats.max_fall_speed);

        if (!fighter->state.is_attacking && !fighter->state.is_stunned) {
            fighter->state.action = (fighter->state.vel.y < 0.0f) ? FIGHTER_ACTION_JUMP : FIGHTER_ACTION_FALL;
        }
    }
}

void fighter_integrate(Fighter* fighter, f32 dt)
{
    if (!fighter->state.alive) {
        return;
    }

    fighter->state.pos.x += fighter->state.vel.x * dt;
    fighter->state.pos.y += fighter->state.vel.y * dt;
}

void fighter_resolve_world_collision(Fighter* fighter, const Room* room, f32 dt)
{
    f32 feet_x1;
    f32 feet_x2;
    f32 feet_y;
    f32 prev_feet_y;
    bool grounded_now;
    bool falling;
    bool hit1;
    bool hit2;
    f32 snap_y1;
    f32 snap_y2;
    f32 snap_y;
    f32 room_max_x;

    if (!fighter || !room || !fighter->state.alive) {
        return;
    }

    grounded_now = false;

    feet_x1 = fighter->state.pos.x + 2.0f;
    feet_x2 = fighter->state.pos.x + PLAYER_WIDTH - 2.0f;
    feet_y = fighter->state.pos.y + PLAYER_HEIGHT;
    prev_feet_y = feet_y - (fighter->state.vel.y * dt);
    falling = (fighter->state.vel.y >= 0.0f);

    hit1 = room_ground_contact_at(room, feet_x1, feet_y, prev_feet_y, falling, &snap_y1);
    hit2 = room_ground_contact_at(room, feet_x2, feet_y, prev_feet_y, falling, &snap_y2);

    if (hit1 || hit2) {
        if (hit1 && hit2) {
            snap_y = (snap_y1 < snap_y2) ? snap_y1 : snap_y2;
        } else {
            snap_y = hit1 ? snap_y1 : snap_y2;
        }

        fighter->state.pos.y = snap_y;
        fighter->state.vel.y = 0.0f;
        grounded_now = true;
    }

    if (grounded_now) {
        if (!fighter->state.grounded) {
            fighter->state.action = (fighter->state.vel.x != 0.0f) ? FIGHTER_ACTION_RUN : FIGHTER_ACTION_IDLE;
        }
        fighter->state.grounded = true;
        fighter->state.coyote_time = DEFAULT_COYOTE_TIME;
    } else {
        if (fighter->state.grounded) {
            fighter->state.action = FIGHTER_ACTION_FALL;
        }
        fighter->state.grounded = false;
    }

    if (fighter->state.pos.x < 0.0f) {
        fighter->state.pos.x = 0.0f;
    }

    room_max_x = (f32)(room->width_tiles * TILE_SIZE) - PLAYER_WIDTH;
    if (fighter->state.pos.x > room_max_x) {
        fighter->state.pos.x = room_max_x;
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

static void apply_parry_or_hit(Fighter* attacker, Fighter* defender)
{
    bool attacker_armed = attacker->state.has_sword;
    bool defender_has_matching_sword =
        defender->state.has_sword &&
        defender->state.sword_line == attacker->state.sword_line;

    if (!fighters_in_attack_range(attacker, defender, attacker_armed)) {
        return;
    }

    if (attacker_armed) {
        if (defender_has_matching_sword) {
            attacker->state.has_sword = false;
            attacker->state.is_stunned = true;
            attacker->state.stun_timer = DEFAULT_STUN_DURATION * 0.35f;
            attacker->state.action = FIGHTER_ACTION_STUN;
            attacker->state.vel.x = attacker->state.facing_right
                ? -(attacker->stats.move_speed * 0.8f)
                : (attacker->stats.move_speed * 0.8f);
            attacker->state.vel.y = -140.0f;
            defender->state.is_parrying = true;
        } else {
            defender->state.alive = false;
            defender->state.action = FIGHTER_ACTION_DEAD;
        }
    } else {
        defender->state.is_stunned = true;
        defender->state.stun_timer = DEFAULT_STUN_DURATION;
        defender->state.action = FIGHTER_ACTION_STUN;
    }
}

void combat_resolve_attacks(CombatState* combat)
{
    Fighter* p1 = &combat->fighters[0];
    Fighter* p2 = &combat->fighters[1];

    if (p1->state.alive && p2->state.alive) {
        if (p1->state.is_attacking) {
            apply_parry_or_hit(p1, p2);
            p1->state.is_attacking = false;
        }
    }

    if (p1->state.alive && p2->state.alive) {
        if (p2->state.is_attacking) {
            apply_parry_or_hit(p2, p1);
            p2->state.is_attacking = false;
        }
    }

    if (!p1->state.alive && p2->state.alive) {
        combat->round_over = true;
        combat->winner_index = 1;
    } else if (!p2->state.alive && p1->state.alive) {
        combat->round_over = true;
        combat->winner_index = 0;
    }
}

void combat_check_fall_deaths(CombatState* combat, const Room* room)
{
    f32 death_y = (f32)(room->height_tiles * TILE_SIZE + 200);

    if (combat->fighters[0].state.pos.y > death_y) {
        combat->fighters[0].state.alive = false;
        combat->round_over = true;
        combat->winner_index = 1;
    }

    if (combat->fighters[1].state.pos.y > death_y) {
        combat->fighters[1].state.alive = false;
        combat->round_over = true;
        combat->winner_index = 0;
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

    fighter_apply_gravity(p1, dt);
    fighter_apply_gravity(p2, dt);

    fighter_integrate(p1, dt);
    fighter_integrate(p2, dt);

    fighter_resolve_world_collision(p1, room, dt);
    fighter_resolve_world_collision(p2, room, dt);

    combat_resolve_facing(p1, p2);
    combat_resolve_attacks(combat);
    combat_check_fall_deaths(combat, room);
}
