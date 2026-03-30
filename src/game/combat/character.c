#include "combat_internal.h"
#include "utils/utils.h"
#include <math.h>

static FighterAction sword_line_to_attack_action(SwordLine line)
{
    switch (line) {
        case SWORD_LINE_HIGH: return FIGHTER_ACTION_ATTACK_HIGH;
        case SWORD_LINE_LOW: return FIGHTER_ACTION_ATTACK_LOW;
        case SWORD_LINE_MID:
        default: return FIGHTER_ACTION_ATTACK_MID;
    }
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

void fighter_update_timers(Fighter* fighter, f32 dt)
{
    if (!fighter->state.alive) {
        return;
    }

    if (fighter->state.attack_cooldown > 0.0f) {
        fighter->state.attack_cooldown -= dt;
        clamp_min_zero_f32(&fighter->state.attack_cooldown);
    }

    if (fighter->state.thrust_buffer_timer > 0.0f) {
        fighter->state.thrust_buffer_timer -= dt;
        clamp_min_zero_f32(&fighter->state.thrust_buffer_timer);
    }

    if (fighter->state.stun_timer > 0.0f) {
        fighter->state.stun_timer -= dt;
        clamp_min_zero_f32(&fighter->state.stun_timer);
        if (fighter->state.stun_timer == 0.0f) {
            fighter->state.is_stunned = false;
            fighter->state.action = FIGHTER_ACTION_IDLE;
        }
    }

    if (fighter->state.invincibility_timer > 0.0f) {
        fighter->state.invincibility_timer -= dt;
        clamp_min_zero_f32(&fighter->state.invincibility_timer);
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
        clamp_min_zero_f32(&fighter->state.coyote_time);
    }
}

void fighter_apply_command(Fighter* fighter, const PlayerCommand* cmd, f32 dt)
{
    bool running;
    f32 desired_walk_vx;
    f32 step_speed_cap;
    f32 thrust_lunge;

    if (!fighter || !cmd || !fighter->state.alive) {
        return;
    }

    if (fighter->state.is_stunned) {
        fighter->state.vel.x = 0.0f;
        fighter->state.move_hold_time = 0.0f;
        return;
    }

    if (cmd->up_pressed) {
        SwordLine line = clamp(fighter->state.sword_line-1,0,2);
        fighter->state.sword_line = line;
    }

    if (cmd->down_pressed) {
        SwordLine line = clamp(fighter->state.sword_line+1,0,2);
        fighter->state.sword_line = line;
    }

    if (cmd->thrust_pressed && fighter->state.grounded) {
        fighter->state.thrust_buffer_timer = THRUST_INPUT_BUFFER_TIME;
    }

    if (fighter->state.is_attacking || fighter->state.action == FIGHTER_ACTION_THROW) {
        fighter->state.move_hold_time = 0.0f;
        fighter->state.vel.x *= THRUST_MOMENTUM_DAMP_MULTIPLIER;
        if (fabsf(fighter->state.vel.x) < 1.0f) {
            fighter->state.vel.x = 0.0f;
        }
        return;
    }
    i8 move_x = cmd->right_pressed + (-cmd->left_pressed);
    if (fighter->state.grounded && move_x != 0) {
        fighter->state.move_hold_time += dt;
    } else {
        fighter->state.move_hold_time = 0.0f;
    }

    running = fighter->state.grounded && fighter->state.move_hold_time >= RUN_HOLSTER_DELAY;
    if (running) {
        fighter->state.vel.x = (f32)move_x * fighter->stats.move_speed;
    } else {
        desired_walk_vx = (f32)move_x * fighter->stats.move_speed * WALK_STEP_SPEED_MULTIPLIER;
        fighter->state.vel.x = desired_walk_vx + (fighter->state.vel.x * WALK_STEP_MOMENTUM_BLEND);

        step_speed_cap = fighter->stats.move_speed * WALK_STEP_SPEED_MULTIPLIER * WALK_STEP_MAX_BOOST_MULTIPLIER;
        fighter->state.vel.x = clampf(fighter->state.vel.x, -step_speed_cap, step_speed_cap);

        if ((move_x > 0 && fighter->state.vel.x < 0.0f) || (move_x < 0 && fighter->state.vel.x > 0.0f)) {
            fighter->state.vel.x = desired_walk_vx;
        }
    }

    if (move_x != 0) {
        fighter->state.facing_right = (move_x > 0);
        if (running && !fighter->state.is_attacking && fighter->state.action != FIGHTER_ACTION_ROLL) {
            fighter->state.action = FIGHTER_ACTION_RUN;
        } else if (fighter->state.grounded && !fighter->state.is_attacking && fighter->state.action != FIGHTER_ACTION_ROLL) {
            fighter->state.action = FIGHTER_ACTION_IDLE;
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

    if (fighter->state.thrust_buffer_timer > 0.0f && fighter->state.attack_cooldown <= 0.0f) {
        fighter->state.move_hold_time = 0.0f;
        fighter->state.vel.x *= THRUST_MOMENTUM_START_MULTIPLIER;
        thrust_lunge = fighter->stats.move_speed * THRUST_LUNGE_SPEED_MULTIPLIER;
        fighter->state.vel.x += fighter->state.facing_right ? thrust_lunge : -thrust_lunge;
        fighter->state.vel.x = clampf(
            fighter->state.vel.x,
            -fighter->stats.move_speed * THRUST_LUNGE_MAX_MULTIPLIER,
            fighter->stats.move_speed * THRUST_LUNGE_MAX_MULTIPLIER
        );
        fighter->state.is_attacking = true;
        fighter->state.action = sword_line_to_attack_action(fighter->state.sword_line);
        fighter->state.action_time = 0.0f;
        fighter->state.action_duration = DEFAULT_ATTACK_DURATION * ATTACK_DURATION_MULTIPLIER;
        fighter->state.attack_cooldown = DEFAULT_ATTACK_COOLDOWN;
        fighter->state.thrust_buffer_timer = 0.0f;
    }
}
