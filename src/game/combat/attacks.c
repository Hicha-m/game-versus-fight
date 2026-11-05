#include <math.h>

#include "combat_internal.h"

void combat_emit(CombatState *combat, CombatEventType type, i32 actor, i32 target)
{
    if (combat->event_count < COMBAT_MAX_EVENTS)
        combat->events[combat->event_count++] = (CombatEvent){type, actor, target};
}

void combat_drop_sword(CombatState *combat, Vec2 pos)
{
    for (i32 i = 0; i < MAX_PLAYERS; ++i)
        if (!combat->ground_swords[i].active)
        {
            combat->ground_swords[i] = (GroundSword){true, pos};
            return;
        }
    /* Replace the oldest slot when repeated respawns leave spare weapons. */
    combat->ground_swords[0] = (GroundSword){true, pos};
}

void combat_record_kill(CombatState *combat, i32 attacker_index, i32 victim_index,
                        bool valid_for_progress)
{
    if (!combat || victim_index < 0 || victim_index >= MAX_PLAYERS)
        return;
    Fighter *victim = &combat->fighters[victim_index];
    if (victim->state.has_sword)
        combat_drop_sword(combat, victim->state.pos);
    victim->state.has_sword = false;
    victim->state.alive = false;
    victim->state.is_attacking = false;
    victim->state.is_stunned = false;
    victim->state.animation_time = 0;
    victim->state.action = FIGHTER_ACTION_DEAD;
    victim->state.vel = (Vec2){0, 0};
    combat_emit(combat, COMBAT_EVENT_DEATH, attacker_index, victim_index);
    combat->kill_happened = true;
    combat->kill_attacker_index = attacker_index;
    combat->kill_victim_index = victim_index;
    combat->kill_counts_for_progress = valid_for_progress;
    combat->kill_attacker_timer = 0;
}

f32 combat_sword_reach(const Fighter *fighter)
{
    if (!fighter->state.has_sword || !fighter->state.alive || fighter->state.is_stunned ||
        fighter->state.action == FIGHTER_ACTION_RUN ||
        fighter->state.action == FIGHTER_ACTION_ROLL ||
        fighter->state.action == FIGHTER_ACTION_THROW || !fighter->state.grounded ||
        fighter->state.crouching || fighter->state.throw_ready)
        return 0;
    if (!fighter->state.is_attacking)
        return SWORD_BODY_REACH_X + (fighter->state.sword_line == SWORD_LINE_LOW ? 6.0f : 0);
    f32 ratio = fighter->state.action_duration > 0
                    ? fighter->state.action_time / fighter->state.action_duration
                    : 0.5f;
    /* Anticipation 0–20%, extension/contact 20–65%, recovery thereafter. */
    if (ratio < 0.2f || ratio > 0.65f)
        return SWORD_BODY_REACH_X;
    f32 extension = fminf(1.0f, (ratio - 0.2f) / 0.15f);
    return SWORD_BODY_REACH_X +
           (SWORD_THRUST_RANGE_X * ATTACK_RANGE_ARMED_MULTIPLIER - SWORD_BODY_REACH_X) * extension;
}

static bool attack_contact(const Fighter *fighter)
{
    if (!fighter->state.is_attacking)
        return false;
    if (fighter->state.action_duration <= 0)
        return true;
    f32 ratio = fighter->state.action_time / fighter->state.action_duration;
    return ratio >= 0.2f && ratio <= 0.65f;
}

static bool fighter_sword_is_lethal(const Fighter *fighter)
{
    return fighter && combat_sword_reach(fighter) > 0;
}

f32 combat_sword_y(const Fighter *fighter)
{
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

static f32 sword_base_world_x(const Fighter *fighter)
{
    if (fighter->state.facing_right)
    {
        return fighter->state.pos.x + PLAYER_WIDTH;
    }

    return fighter->state.pos.x;
}

static bool sword_segment_hits_body(const Fighter *sword_owner, const Fighter *target, f32 reach)
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

    if (!sword_owner || !target)
    {
        return false;
    }

    lane_y = combat_sword_y(sword_owner);
    RectF body = fighter_get_body_rect(target);
    body_top = body.y + 3;
    body_bottom = body.y + body.h - 3;
    if (lane_y < body_top || lane_y > body_bottom)
    {
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

    if (sword_max_x < body_min_x)
    {
        return false;
    }

    if (body_max_x < sword_min_x)
    {
        return false;
    }

    return true;
}

static void try_apply_running_impale(CombatState *combat, Fighter *runner,
                                     const Fighter *defender_with_sword, i32 runner_index,
                                     i32 defender_index)
{
    bool moving_toward;
    f32 impale_reach;

    if (!runner || !defender_with_sword)
    {
        return;
    }

    if (!runner->state.alive || runner->state.is_attacking ||
        runner->state.invincibility_timer > 0 ||
        (runner->state.grounded && fabsf(runner->state.vel.x) < 1 && !runner->state.crouching))
    {
        return;
    }

    if (!fighter_sword_is_lethal(defender_with_sword))
    {
        return;
    }

    moving_toward =
        !runner->state.grounded ||
        (runner->state.vel.x > 0.0f && runner->state.pos.x < defender_with_sword->state.pos.x) ||
        (runner->state.vel.x < 0.0f && runner->state.pos.x > defender_with_sword->state.pos.x);
    if (!moving_toward)
    {
        return;
    }

    if (fighter_sword_is_lethal(runner) &&
        fabsf(combat_sword_y(runner) - combat_sword_y(defender_with_sword)) < 6 &&
        runner->state.facing_right != defender_with_sword->state.facing_right)
        return;
    impale_reach = combat_sword_reach(defender_with_sword);
    if (!sword_segment_hits_body(defender_with_sword, runner, impale_reach))
    {
        return;
    }

    runner->state.alive = false;
    runner->state.action = FIGHTER_ACTION_DEAD;
    runner->state.vel.x = 0.0f;
    runner->state.vel.y = 0.0f;
    combat_record_kill(combat, defender_index, runner_index, true);
}

bool fighters_in_attack_range(const Fighter *attacker, const Fighter *defender, bool armed)
{
    if (armed)
        return sword_segment_hits_body(attacker, defender, combat_sword_reach(attacker));
    f32 dx = defender->state.pos.x - attacker->state.pos.x;
    return fabsf(dx) <= SWORD_THRUST_RANGE_X * ATTACK_RANGE_UNARMED_MULTIPLIER &&
           fabsf(defender->state.pos.y - attacker->state.pos.y) <=
               SWORD_HIT_RANGE_Y * ATTACK_HEIGHT_UNARMED_MULTIPLIER &&
           (attacker->state.facing_right ? dx >= 0 : dx <= 0);
}

static void apply_parry_or_hit(CombatState *combat, Fighter *attacker, Fighter *defender,
                               i32 attacker_index, i32 defender_index)
{
    bool special = attacker->state.action == FIGHTER_ACTION_SWEEP ||
                   attacker->state.action == FIGHTER_ACTION_DIVE_KICK;
    bool attacker_armed = !special && fighter_sword_is_lethal(attacker);
    bool defender_has_matching_sword =
        fighter_sword_is_lethal(defender) &&
        fabsf(combat_sword_y(defender) - combat_sword_y(attacker)) <= 6.0f &&
        (defender->state.facing_right ? attacker->state.pos.x > defender->state.pos.x
                                      : attacker->state.pos.x < defender->state.pos.x);

    if (!attack_contact(attacker) || attacker->state.is_stunned ||
        defender->state.invincibility_timer > 0 ||
        !fighters_in_attack_range(attacker, defender, attacker_armed))
    {
        return;
    }

    if (attacker_armed)
    {
        if (defender_has_matching_sword)
        {
            /* A static same-height guard blocks; only a fresh blade crossing disarms. */
            attacker->state.is_attacking = false;
            attacker->state.vel.x = attacker->state.facing_right ? -90.0f : 90.0f;
            defender->state.vel.x = -attacker->state.vel.x;
            combat_emit(combat, COMBAT_EVENT_PARRY, defender_index, attacker_index);
            defender->state.is_parrying = true;
        }
        else
        {
            defender->state.alive = false;
            defender->state.action = FIGHTER_ACTION_DEAD;
            combat_record_kill(combat, attacker_index, defender_index, true);
        }
    }
    else
    {
        if (defender->state.knocked_down)
        {
            if (attacker->state.crouching ||
                fabsf(attacker->state.pos.x - defender->state.pos.x) < 24)
                combat_record_kill(combat, attacker_index, defender_index, true);
            attacker->state.is_attacking = false;
            return;
        }
        if (defender->state.has_sword)
        {
            combat_drop_sword(combat, defender->state.pos);
            defender->state.has_sword = false;
            combat_emit(combat, COMBAT_EVENT_DISARM, attacker_index, defender_index);
        }
        defender->state.punch_hits++;
        defender->state.punch_timer = 0.8f;
        defender->state.knocked_down = special || defender->state.punch_hits >= 2;
        defender->state.is_stunned = true;
        defender->state.is_attacking = false;
        defender->state.stun_timer = defender->state.knocked_down ? 0.7f : 0.16f;
        defender->state.action =
            defender->state.knocked_down ? FIGHTER_ACTION_KNOCKED_DOWN : FIGHTER_ACTION_STUN;
        defender->state.vel.x = attacker->state.facing_right ? 120 : -120;
        attacker->state.is_attacking = false;
    }
}

static void resolve_projectile_hits(CombatState *combat, ThrownSword *sword)
{
    i32 i;

    if (!combat)
    {
        return;
    }

    if (!sword->active)
    {
        return;
    }

    for (i = 0; i < MAX_PLAYERS; ++i)
    {
        Fighter *target;
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

        if (i == sword->owner_index && sword->age < 0.12f)
        {
            continue;
        }

        target = &combat->fighters[i];
        if (!target->state.alive || target->state.invincibility_timer > 0.0f)
        {
            continue;
        }

        proj_x0 = sword->pos.x - THROW_PROJECTILE_WIDTH * 0.5f;
        proj_y0 = sword->pos.y - THROW_PROJECTILE_HEIGHT * 0.5f;
        proj_x1 = proj_x0 + THROW_PROJECTILE_WIDTH;
        proj_y1 = proj_y0 + THROW_PROJECTILE_HEIGHT;

        RectF body = fighter_get_body_rect(target);
        body_x0 = body.x;
        body_y0 = body.y;
        body_x1 = body_x0 + body.w;
        body_y1 = body_y0 + body.h;

        overlap =
            !(proj_x1 < body_x0 || body_x1 < proj_x0 || proj_y1 < body_y0 || body_y1 < proj_y0);
        if (!overlap)
        {
            continue;
        }

        defender_parry = fighter_sword_is_lethal(target) &&
                         target->state.sword_line != SWORD_LINE_LOW &&
                         fabsf(combat_sword_y(target) - sword->pos.y) <= 24 &&
                         (target->state.facing_right ? sword->vel.x < 0 : sword->vel.x > 0);
        if (defender_parry)
        {
            target->state.is_parrying = true;
            combat_drop_sword(combat, (Vec2){sword->pos.x, sword->pos.y - PLAYER_HEIGHT});
            combat_emit(combat, COMBAT_EVENT_PARRY, i, sword->owner_index);
            sword->active = false;
            return;
        }

        target->state.alive = false;
        target->state.action = FIGHTER_ACTION_DEAD;
        target->state.vel.x = 0.0f;
        target->state.vel.y = 0.0f;

        attacker_index =
            (sword->owner_index >= 0 && sword->owner_index < MAX_PLAYERS) ? sword->owner_index : -1;
        combat_record_kill(combat, attacker_index, i, attacker_index >= 0);

        sword->active = false;
        return;
    }
}

void combat_resolve_thrown_sword_hits(CombatState *combat)
{
    if (!combat)
        return;
    resolve_projectile_hits(combat, &combat->thrown_sword);
    resolve_projectile_hits(combat, &combat->second_thrown_sword);
}

static bool guard_crossing(const Fighter *actor, const Fighter *target)
{
    if (!actor->state.guard_changed || !fighter_sword_is_lethal(actor) ||
        !fighter_sword_is_lethal(target))
        return false;
    if (actor->state.facing_right == target->state.facing_right ||
        fabsf(combat_sword_y(actor) - combat_sword_y(target)) > 6)
        return false;
    f32 separation = fabsf(actor->state.pos.x - target->state.pos.x) - PLAYER_WIDTH;
    return separation >= 0 &&
           separation < combat_sword_reach(actor) + combat_sword_reach(target) - 12;
}

void combat_resolve_attacks(CombatState *combat)
{
    Fighter *p1 = &combat->fighters[0];
    Fighter *p2 = &combat->fighters[1];

    bool crossing[2] = {guard_crossing(p1, p2), guard_crossing(p2, p1)};
    if (crossing[0] != crossing[1])
    {
        i32 actor = crossing[0] ? 0 : 1, target = 1 - actor;
        combat_drop_sword(combat, combat->fighters[target].state.pos);
        combat->fighters[target].state.has_sword = false;
        combat->fighters[target].state.is_attacking = false;
        combat_emit(combat, COMBAT_EVENT_DISARM, actor, target);
        return;
    }
    if (p1->state.alive && p2->state.alive)
    {
        try_apply_running_impale(combat, p1, p2, 0, 1);
    }

    if (p1->state.alive && p2->state.alive)
    {
        try_apply_running_impale(combat, p2, p1, 1, 0);
    }

    if (!p1->state.alive || !p2->state.alive)
        return;
    CombatState first = *combat, second = *combat;
    first.event_count = second.event_count = 0;
    if (p1->state.is_attacking)
        apply_parry_or_hit(&first, &first.fighters[0], &first.fighters[1], 0, 1);
    if (p2->state.is_attacking)
        apply_parry_or_hit(&second, &second.fighters[1], &second.fighters[0], 1, 0);
    /* Each direction observes the same tick snapshot. Merge effects by recipient. */
    for (i32 i = 0; i < MAX_PLAYERS; ++i)
    {
        FighterState left = first.fighters[i].state, right = second.fighters[i].state;
        if (!left.alive)
            combat->fighters[i].state = left;
        else if (!right.alive)
            combat->fighters[i].state = right;
        else if (left.is_stunned || !left.has_sword)
            combat->fighters[i].state = left;
        else if (right.is_stunned || !right.has_sword)
            combat->fighters[i].state = right;
        else
        {
            combat->fighters[i].state.is_parrying = left.is_parrying || right.is_parrying;
            combat->fighters[i].state.is_attacking = left.is_attacking && right.is_attacking;
        }
    }
    const CombatState *results[2] = {&first, &second};
    for (i32 k = 0; k < 2; ++k)
        for (i32 i = 0; i < results[k]->event_count; ++i)
        {
            CombatEvent event = results[k]->events[i];
            combat_emit(combat, event.type, event.actor, event.target);
            if (event.type == COMBAT_EVENT_DISARM || event.type == COMBAT_EVENT_DEATH)
            {
                const Fighter *original =
                    k == 0 ? &second.fighters[event.target] : &first.fighters[event.target];
                if (original->state.has_sword)
                    combat_drop_sword(combat, original->state.pos);
            }
            if (event.type == COMBAT_EVENT_DEATH)
            {
                combat->kill_happened = true;
                combat->kill_attacker_index = event.actor;
                combat->kill_victim_index = event.target;
                combat->kill_counts_for_progress = true;
            }
        }
    if (!p1->state.alive && !p2->state.alive)
        combat->kill_counts_for_progress = false;
}

void combat_check_fall_deaths(CombatState *combat, const Room *room)
{
    f32 death_y = (f32)((room->height_tiles) * TILE_SIZE - PLAYER_HEIGHT);

    if (combat->fighters[0].state.alive && combat->fighters[0].state.pos.y > death_y)
    {
        combat->fighters[0].state.alive = false;
        combat->fighters[0].state.action = FIGHTER_ACTION_DEAD;
        combat_record_kill(combat, -1, 0, false);
    }

    if (combat->fighters[1].state.alive && combat->fighters[1].state.pos.y > death_y)
    {
        combat->fighters[1].state.alive = false;
        combat->fighters[1].state.action = FIGHTER_ACTION_DEAD;
        combat_record_kill(combat, -1, 1, false);
    }
}
