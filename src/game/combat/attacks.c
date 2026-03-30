#include <math.h>

#include "combat_internal.h"

static void combat_record_kill(CombatState* combat, i32 attacker_index, i32 victim_index, bool valid_for_progress)
{
    if (!combat) {
        return;
    }

    combat->kill_happened = true;
    combat->kill_attacker_index = attacker_index;
    combat->kill_victim_index = victim_index;
    combat->kill_counts_for_progress = valid_for_progress;
}

static bool fighter_sword_is_lethal(const Fighter* fighter)
{
    if (!fighter || !fighter->state.alive || !fighter->state.has_sword) {
        return false;
    }

    if (fighter->state.action == FIGHTER_ACTION_RUN || fighter->state.action == FIGHTER_ACTION_ROLL) {
        return false;
    }

    return true;
}

static f32 sword_line_world_y(const Fighter* fighter)
{
    if (fighter->state.sword_line == SWORD_LINE_HIGH) {
        return fighter->state.pos.y + 8.0f;
    }
    if (fighter->state.sword_line == SWORD_LINE_LOW) {
        return fighter->state.pos.y + PLAYER_HEIGHT - 10.0f;
    }

    return fighter->state.pos.y + PLAYER_HEIGHT * 0.5f;
}

static f32 sword_base_world_x(const Fighter* fighter)
{
    if (fighter->state.facing_right) {
        return fighter->state.pos.x + PLAYER_WIDTH;
    }

    return fighter->state.pos.x;
}

static bool sword_segment_hits_body(const Fighter* sword_owner, const Fighter* target, f32 reach)
{
    f32 lane_y;
    f32 body_top;
    f32 body_bottom;
    f32 sword_x0;
    f32 sword_x1;
    f32 body_x0;
    f32 body_x1;
    f32 sword_min_x;
    f32 sword_max_x;
    f32 body_min_x;
    f32 body_max_x;

    if (!sword_owner || !target) {
        return false;
    }

    lane_y = sword_line_world_y(sword_owner);
    body_top = target->state.pos.y + 6.0f;
    body_bottom = target->state.pos.y + PLAYER_HEIGHT - 6.0f;
    if (lane_y < body_top || lane_y > body_bottom) {
        return false;
    }

    sword_x0 = sword_base_world_x(sword_owner);
    sword_x1 = sword_x0 + (sword_owner->state.facing_right ? reach : -reach);
    body_x0 = target->state.pos.x;
    body_x1 = target->state.pos.x + PLAYER_WIDTH;

    sword_min_x = (sword_x0 < sword_x1) ? sword_x0 : sword_x1;
    sword_max_x = (sword_x0 > sword_x1) ? sword_x0 : sword_x1;
    body_min_x = (body_x0 < body_x1) ? body_x0 : body_x1;
    body_max_x = (body_x0 > body_x1) ? body_x0 : body_x1;

    if (sword_max_x < body_min_x) {
        return false;
    }

    if (body_max_x < sword_min_x) {
        return false;
    }

    return true;
}

static void try_apply_running_impale(
    CombatState* combat,
    Fighter* runner,
    const Fighter* defender_with_sword,
    i32 runner_index,
    i32 defender_index
)
{
    bool moving_toward;
    f32 impale_reach;

    if (!runner || !defender_with_sword) {
        return;
    }

    if (!runner->state.alive || runner->state.action != FIGHTER_ACTION_RUN) {
        return;
    }

    if (!fighter_sword_is_lethal(defender_with_sword)) {
        return;
    }

    moving_toward =
        (runner->state.vel.x > 0.0f && runner->state.pos.x < defender_with_sword->state.pos.x) ||
        (runner->state.vel.x < 0.0f && runner->state.pos.x > defender_with_sword->state.pos.x);
    if (!moving_toward) {
        return;
    }

    impale_reach = SWORD_BODY_REACH_X;
    if (!sword_segment_hits_body(defender_with_sword, runner, impale_reach)) {
        return;
    }

    runner->state.alive = false;
    runner->state.action = FIGHTER_ACTION_DEAD;
    runner->state.vel.x = 0.0f;
    runner->state.vel.y = 0.0f;
    combat_record_kill(combat, defender_index, runner_index, true);
}

bool fighters_in_attack_range(const Fighter* attacker, const Fighter* defender, bool armed)
{
    f32 dx;
    f32 dy;
    f32 x_range;
    f32 y_range;
    bool facing_ok;

    dx = defender->state.pos.x - attacker->state.pos.x;
    dy = fabsf(defender->state.pos.y - attacker->state.pos.y);

    x_range = armed ? (SWORD_THRUST_RANGE_X * ATTACK_RANGE_ARMED_MULTIPLIER)
                    : (SWORD_THRUST_RANGE_X * ATTACK_RANGE_UNARMED_MULTIPLIER);
    y_range = armed ? (SWORD_HIT_RANGE_Y * ATTACK_HEIGHT_ARMED_MULTIPLIER)
                    : (SWORD_HIT_RANGE_Y * ATTACK_HEIGHT_UNARMED_MULTIPLIER);
    facing_ok = false;

    if (attacker->state.facing_right && dx >= 0.0f && dx <= x_range) {
        facing_ok = true;
    } else if (!attacker->state.facing_right && dx <= 0.0f && fabsf(dx) <= x_range) {
        facing_ok = true;
    }

    return facing_ok && (dy <= y_range);
}

static void apply_parry_or_hit(CombatState* combat, Fighter* attacker, Fighter* defender, i32 attacker_index, i32 defender_index)
{
    bool attacker_armed = fighter_sword_is_lethal(attacker);
    bool defender_has_matching_sword =
        fighter_sword_is_lethal(defender) &&
        defender->state.sword_line == attacker->state.sword_line;

    if (!fighters_in_attack_range(attacker, defender, attacker_armed)) {
        return;
    }

    if (attacker_armed) {
        if (defender_has_matching_sword) {
            attacker->state.has_sword = false;
            attacker->state.is_stunned = true;
            attacker->state.stun_timer = DEFAULT_STUN_DURATION * PARRY_STUN_MULTIPLIER;
            attacker->state.action = FIGHTER_ACTION_STUN;
            attacker->state.vel.x = attacker->state.facing_right
                ? -(attacker->stats.move_speed * PARRY_KNOCKBACK_MULTIPLIER)
                : (attacker->stats.move_speed * PARRY_KNOCKBACK_MULTIPLIER);
            attacker->state.vel.y = PARRY_VERTICAL_RECOIL;
            defender->state.is_parrying = true;
        } else {
            defender->state.alive = false;
            defender->state.action = FIGHTER_ACTION_DEAD;
            combat_record_kill(combat, attacker_index, defender_index, true);
        }
    } else {
        defender->state.is_stunned = true;
        defender->state.stun_timer = DEFAULT_STUN_DURATION;
        defender->state.action = FIGHTER_ACTION_STUN;
    }
}

void combat_resolve_thrown_sword_hits(CombatState* combat)
{
    ThrownSword* sword;
    i32 i;

    if (!combat) {
        return;
    }

    sword = &combat->thrown_sword;
    if (!sword->active) {
        return;
    }

    for (i = 0; i < MAX_PLAYERS; ++i) {
        Fighter* target;
        f32 proj_x0;
        f32 proj_y0;
        f32 proj_x1;
        f32 proj_y1;
        f32 body_x0;
        f32 body_y0;
        f32 body_x1;
        f32 body_y1;
        bool overlap;
        bool defender_parry;
        i32 attacker_index;

        if (i == sword->owner_index) {
            continue;
        }

        target = &combat->fighters[i];
        if (!target->state.alive || target->state.invincibility_timer > 0.0f) {
            continue;
        }

        proj_x0 = sword->pos.x - THROW_PROJECTILE_WIDTH * 0.5f;
        proj_y0 = sword->pos.y - THROW_PROJECTILE_HEIGHT * 0.5f;
        proj_x1 = proj_x0 + THROW_PROJECTILE_WIDTH;
        proj_y1 = proj_y0 + THROW_PROJECTILE_HEIGHT;

        body_x0 = target->state.pos.x;
        body_y0 = target->state.pos.y;
        body_x1 = body_x0 + PLAYER_WIDTH;
        body_y1 = body_y0 + PLAYER_HEIGHT;

        overlap = !(proj_x1 < body_x0 || body_x1 < proj_x0 || proj_y1 < body_y0 || body_y1 < proj_y0);
        if (!overlap) {
            continue;
        }

        defender_parry = fighter_sword_is_lethal(target) && target->state.sword_line == sword->line;
        if (defender_parry) {
            target->state.is_parrying = true;
            sword->active = false;
            return;
        }

        target->state.alive = false;
        target->state.action = FIGHTER_ACTION_DEAD;
        target->state.vel.x = 0.0f;
        target->state.vel.y = 0.0f;

        attacker_index = (sword->owner_index >= 0 && sword->owner_index < MAX_PLAYERS)
            ? sword->owner_index
            : -1;
        combat_record_kill(combat, attacker_index, i, attacker_index >= 0);

        sword->active = false;
        return;
    }
}

void combat_resolve_attacks(CombatState* combat)
{
    Fighter* p1 = &combat->fighters[0];
    Fighter* p2 = &combat->fighters[1];

    if (p1->state.alive && p2->state.alive) {
        try_apply_running_impale(combat, p1, p2, 0, 1);
    }

    if (p1->state.alive && p2->state.alive) {
        try_apply_running_impale(combat, p2, p1, 1, 0);
    }

    if (p1->state.alive && p2->state.alive) {
        if (p1->state.is_attacking) {
            apply_parry_or_hit(combat, p1, p2, 0, 1);
        }
    }

    if (p1->state.alive && p2->state.alive) {
        if (p2->state.is_attacking) {
            apply_parry_or_hit(combat, p2, p1, 1, 0);
        }
    }
}

void combat_check_fall_deaths(CombatState* combat, const Room* room)
{
    f32 death_y = (f32)((room->height_tiles) * TILE_SIZE - PLAYER_HEIGHT);

    if (combat->fighters[0].state.alive && combat->fighters[0].state.pos.y > death_y) {
        combat->fighters[0].state.alive = false;
        combat->fighters[0].state.action = FIGHTER_ACTION_DEAD;
        combat_record_kill(combat, -1, 0, false);
    }

    if (combat->fighters[1].state.alive && combat->fighters[1].state.pos.y > death_y) {
        combat->fighters[1].state.alive = false;
        combat->fighters[1].state.action = FIGHTER_ACTION_DEAD;
        combat_record_kill(combat, -1, 1, false);
    }
}
