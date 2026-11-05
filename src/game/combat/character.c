#include "combat_internal.h"
#include "utils/utils.h"
#include <math.h>

static FighterAction sword_line_to_attack_action(SwordLine line)
{
    switch (line)
    {
    case SWORD_LINE_HIGH:
        return FIGHTER_ACTION_ATTACK_HIGH;
    case SWORD_LINE_LOW:
        return FIGHTER_ACTION_ATTACK_LOW;
    case SWORD_LINE_MID:
    default:
        return FIGHTER_ACTION_ATTACK_MID;
    }
}

RectF fighter_get_body_rect(const Fighter *fighter)
{
    RectF r;
    r.x = fighter->state.pos.x;
    bool low = fighter->state.crouching || fighter->state.action == FIGHTER_ACTION_ROLL ||
               fighter->state.knocked_down;
    r.y = fighter->state.pos.y + (low ? PLAYER_HEIGHT * 0.5f : 0);
    r.w = PLAYER_WIDTH;
    r.h = low ? PLAYER_HEIGHT * 0.5f : PLAYER_HEIGHT;
    return r;
}

void fighter_update_timers(Fighter *fighter, f32 dt)
{
    fighter->state.animation_time += dt;
    if (!fighter->state.alive)
        return;
    fighter->state.guard_changed = false;
    fighter->state.wall_jump_lock = fmaxf(0, fighter->state.wall_jump_lock - dt);
    fighter->state.punch_timer = fmaxf(0, fighter->state.punch_timer - dt);
    if (fighter->state.punch_timer == 0)
        fighter->state.punch_hits = 0;

    if (fighter->state.attack_cooldown > 0.0f)
    {
        fighter->state.attack_cooldown -= dt;
        clamp_min_zero_f32(&fighter->state.attack_cooldown);
    }

    if (fighter->state.action != FIGHTER_ACTION_ROLL && fighter->state.thrust_buffer_timer > 0.0f)
    {
        fighter->state.thrust_buffer_timer -= dt;
        clamp_min_zero_f32(&fighter->state.thrust_buffer_timer);
    }

    if (fighter->state.stun_timer > 0.0f)
    {
        fighter->state.stun_timer -= dt;
        clamp_min_zero_f32(&fighter->state.stun_timer);
        if (fighter->state.stun_timer == 0.0f)
        {
            fighter->state.is_stunned = false;
            fighter->state.knocked_down = false;
            fighter->state.action = FIGHTER_ACTION_IDLE;
        }
    }

    if (fighter->state.invincibility_timer > 0.0f)
    {
        fighter->state.invincibility_timer -= dt;
        clamp_min_zero_f32(&fighter->state.invincibility_timer);
    }

    if (fighter->state.action_duration > 0.0f)
    {
        fighter->state.action_time += dt;
        if (fighter->state.action_time >= fighter->state.action_duration)
        {
            if (fighter->state.action == FIGHTER_ACTION_ROLL && fighter->state.has_sword &&
                fighter->state.sword_line != SWORD_LINE_MID)
                fighter->state.guard_changed = true;
            fighter->state.action_time = 0.0f;
            fighter->state.action_duration = 0.0f;
            fighter->state.is_attacking = false;
            if (fighter->state.grounded && !fighter->state.is_stunned)
            {
                fighter->state.action = FIGHTER_ACTION_IDLE;
            }
        }
    }

    if (!fighter->state.grounded)
    {
        fighter->state.coyote_time -= dt;
        clamp_min_zero_f32(&fighter->state.coyote_time);
    }
}

void fighter_apply_command(Fighter *fighter, const PlayerCommand *cmd, f32 dt)
{
    bool running;
    f32 desired_walk_vx;
    f32 step_speed_cap;
    f32 thrust_lunge;

    if (!fighter || !cmd || !fighter->state.alive)
    {
        return;
    }

    if (fighter->state.knocked_down &&
        (cmd->up_pressed || cmd->left_pressed || cmd->right_pressed) &&
        fighter->state.stun_timer < 0.45f)
    {
        fighter->state.knocked_down = false;
        fighter->state.is_stunned = false;
        fighter->state.stun_timer = 0;
    }
    if (fighter->state.is_stunned)
    {
        fighter->state.vel.x *= 0.96f;
        fighter->state.move_hold_time = 0.0f;
        return;
    }

    if (fighter->state.action == FIGHTER_ACTION_ROLL && fighter->state.action_duration > 0)
        return;

    FighterAction previous_action = fighter->state.action;
    SwordLine previous_line = fighter->state.sword_line;
    fighter->state.climbing = cmd->up_held;
    fighter->state.crouch_hold_time = cmd->crouch_held ? fighter->state.crouch_hold_time + dt : 0;
    fighter->state.up_hold_time = cmd->up_held ? fighter->state.up_hold_time + dt : 0;
    fighter->state.crouching =
        cmd->crouch_held && (fighter->state.crouch_hold_time >= 0.10f || cmd->thrust_pressed);

    if (cmd->set_sword_line && cmd->target_sword_line >= SWORD_LINE_HIGH &&
        cmd->target_sword_line <= SWORD_LINE_LOW)
    {
        fighter->state.sword_line = cmd->target_sword_line;
    }

    if (cmd->up_pressed)
    {
        SwordLine line = clamp(fighter->state.sword_line - 1, 0, 2);
        fighter->state.sword_line = line;
    }

    if (cmd->down_pressed)
    {
        SwordLine line = clamp(fighter->state.sword_line + 1, 0, 2);
        fighter->state.sword_line = line;
    }

    fighter->state.guard_changed =
        fighter->state.guard_changed || previous_line != fighter->state.sword_line;
    fighter->state.throw_ready =
        fighter->state.up_hold_time >= 0.18f && fighter->state.sword_line == SWORD_LINE_HIGH;
    if (cmd->thrust_pressed && !fighter->state.grounded && fighter->state.wall_direction == 0 &&
        !fighter->state.is_attacking)
    {
        fighter->state.action = FIGHTER_ACTION_DIVE_KICK;
        fighter->state.action_time = 0;
        fighter->state.action_duration = 0.4f;
        fighter->state.vel.x =
            (fighter->state.facing_right ? 1 : -1) * fighter->stats.move_speed * 1.15f;
        fighter->state.vel.y = fighter->stats.move_speed * 1.15f;
        fighter->state.is_attacking = true;
        return;
    }
    if (cmd->thrust_pressed)
    {
        fighter->state.thrust_buffer_timer = THRUST_INPUT_BUFFER_TIME;
    }

    if (fighter->state.is_attacking || fighter->state.action == FIGHTER_ACTION_THROW)
    {
        fighter->state.move_hold_time = 0.0f;
        fighter->state.vel.x *= THRUST_MOMENTUM_DAMP_MULTIPLIER;
        if (fabsf(fighter->state.vel.x) < 1.0f)
        {
            fighter->state.vel.x = 0.0f;
        }
        return;
    }
    i8 move_x = cmd->right_pressed + (-cmd->left_pressed);
    if (fighter->state.grounded && move_x != 0)
    {
        fighter->state.move_hold_time += dt;
    }
    else
    {
        fighter->state.move_hold_time = 0.0f;
    }

    running = fighter->state.grounded && fighter->state.move_hold_time >= RUN_HOLSTER_DELAY;
    if (running)
    {
        fighter->state.vel.x = (f32)move_x * fighter->stats.move_speed;
    }
    else
    {
        desired_walk_vx = (f32)move_x * fighter->stats.move_speed * WALK_STEP_SPEED_MULTIPLIER;
        fighter->state.vel.x = desired_walk_vx + (fighter->state.vel.x * WALK_STEP_MOMENTUM_BLEND);

        step_speed_cap =
            fighter->stats.move_speed * WALK_STEP_SPEED_MULTIPLIER * WALK_STEP_MAX_BOOST_MULTIPLIER;
        fighter->state.vel.x = clampf(fighter->state.vel.x, -step_speed_cap, step_speed_cap);

        if ((move_x > 0 && fighter->state.vel.x < 0.0f) ||
            (move_x < 0 && fighter->state.vel.x > 0.0f))
        {
            fighter->state.vel.x = desired_walk_vx;
        }
    }

    if (!fighter->state.grounded)
        fighter->state.vel.x = move_x * fighter->stats.move_speed;

    if (move_x != 0 && fighter->state.wall_jump_lock <= 0)
    {
        fighter->state.facing_right = (move_x > 0);
        if (running && !fighter->state.is_attacking && fighter->state.action != FIGHTER_ACTION_ROLL)
        {
            fighter->state.action = FIGHTER_ACTION_RUN;
        }
        else if (fighter->state.grounded && !fighter->state.is_attacking &&
                 fighter->state.action != FIGHTER_ACTION_ROLL)
        {
            fighter->state.action = FIGHTER_ACTION_IDLE;
        }
    }
    else if (fighter->state.grounded && !fighter->state.is_attacking &&
             fighter->state.action != FIGHTER_ACTION_ROLL)
    {
        fighter->state.action = FIGHTER_ACTION_IDLE;
    }

    if (fighter->state.wall_jump_lock > 0)
        fighter->state.vel.x = (fighter->state.facing_right ? 1 : -1) * fighter->stats.move_speed;
    if (fighter->state.crouching && fighter->state.grounded)
        fighter->state.vel.x *= 0.3f;
    if (!fighter->state.has_sword)
        fighter->state.vel.x *= 1.12f;
    if (!fighter->state.grounded && fighter->state.wall_direction && cmd->jump_pressed)
    {
        fighter->state.vel.x = -fighter->state.wall_direction * fighter->stats.move_speed;
        fighter->state.facing_right = fighter->state.wall_direction < 0;
        fighter->state.vel.y = fighter->stats.jump_speed;
        fighter->state.wall_jump_lock = 0.18f;
        fighter->state.wall_direction = 0;
    }
    else if ((fighter->state.grounded || fighter->state.coyote_time > 0.0f) && cmd->jump_pressed)
    {
        fighter->state.vel.y = fighter->stats.jump_speed;
        fighter->state.grounded = false;
        fighter->state.coyote_time = 0.0f;
        fighter->state.action = FIGHTER_ACTION_JUMP;
    }

    if ((cmd->roll_pressed ||
         (cmd->crouch_held && move_x != 0 && fighter->state.move_hold_time >= RUN_HOLSTER_DELAY)) &&
        fighter->state.grounded && !fighter->state.is_attacking)
    {
        fighter->state.crouching = true;
        fighter->state.vel.x =
            fighter->state.facing_right ? fighter->stats.move_speed : -fighter->stats.move_speed;
        fighter->state.action = FIGHTER_ACTION_ROLL;
        fighter->state.action_duration = DEFAULT_ROLL_DURATION;
        fighter->state.action_time = 0.0f;
    }

    if (previous_action == FIGHTER_ACTION_RUN && !running && fighter->state.has_sword &&
        fighter->state.sword_line != SWORD_LINE_MID)
        fighter->state.guard_changed = true;

    if (fighter->state.action != FIGHTER_ACTION_ROLL && fighter->state.thrust_buffer_timer > 0.0f &&
        fighter->state.attack_cooldown <= 0.0f)
    {
        fighter->state.move_hold_time = 0.0f;
        fighter->state.vel.x *= THRUST_MOMENTUM_START_MULTIPLIER;
        thrust_lunge = fighter->stats.move_speed * THRUST_LUNGE_SPEED_MULTIPLIER;
        fighter->state.vel.x += fighter->state.facing_right ? thrust_lunge : -thrust_lunge;
        fighter->state.vel.x =
            clampf(fighter->state.vel.x, -fighter->stats.move_speed * THRUST_LUNGE_MAX_MULTIPLIER,
                   fighter->stats.move_speed * THRUST_LUNGE_MAX_MULTIPLIER);
        fighter->state.is_attacking = true;
        fighter->state.action = fighter->state.crouching
                                    ? FIGHTER_ACTION_SWEEP
                                    : sword_line_to_attack_action(fighter->state.sword_line);
        fighter->state.action_time = 0.0f;
        fighter->state.action_duration = DEFAULT_ATTACK_DURATION * ATTACK_DURATION_MULTIPLIER;
        fighter->state.attack_cooldown = DEFAULT_ATTACK_COOLDOWN;
        fighter->state.thrust_buffer_timer = 0.0f;
    }
}

void fighter_sync_action(Fighter *fighter)
{
    FighterState *s = &fighter->state;
    if (!s->alive)
    {
        s->action = FIGHTER_ACTION_DEAD;
        s->is_attacking = false;
        s->is_stunned = false;
        return;
    }
    if (s->is_stunned)
    {
        s->action = s->knocked_down ? FIGHTER_ACTION_KNOCKED_DOWN : FIGHTER_ACTION_STUN;
        s->is_attacking = false;
        return;
    }
    if (s->is_attacking)
    {
        if (s->action != FIGHTER_ACTION_DIVE_KICK && s->action != FIGHTER_ACTION_SWEEP)
            s->action = sword_line_to_attack_action(s->sword_line);
        return;
    }
    if (s->action_duration > 0 &&
        (s->action == FIGHTER_ACTION_ROLL || s->action == FIGHTER_ACTION_THROW))
        return;
    if (s->crouching && s->grounded)
    {
        s->action = FIGHTER_ACTION_CROUCH;
        return;
    }
    if (s->wall_direction && !s->grounded)
    {
        s->action = FIGHTER_ACTION_WALL_SLIDE;
        return;
    }
    s->action = !s->grounded ? (s->vel.y < 0 ? FIGHTER_ACTION_JUMP : FIGHTER_ACTION_FALL)
                             : (s->move_hold_time >= RUN_HOLSTER_DELAY ? FIGHTER_ACTION_RUN
                                                                       : FIGHTER_ACTION_IDLE);
}
