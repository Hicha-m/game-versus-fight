#include "combat_internal.h"
#include "utils/utils.h"

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

void combat_update_thrown_sword_physics(CombatState* combat, const Room* room, f32 dt)
{
    ThrownSword* sword;
    i32 tx;
    i32 ty;
    TileType tile;
    f32 room_width_px;
    f32 room_height_px;

    if (!combat || !room) {
        return;
    }

    sword = &combat->thrown_sword;
    if (!sword->active) {
        return;
    }

    sword->lifetime -= dt;
    if (sword->lifetime <= 0.0f) {
        sword->active = false;
        return;
    }

    sword->vel.y += THROW_PROJECTILE_GRAVITY * dt;
    sword->pos.x += sword->vel.x * dt;
    sword->pos.y += sword->vel.y * dt;
    sword->rotation_deg += ((sword->vel.x >= 0.0f) ? 1.0f : -1.0f) * THROW_PROJECTILE_SPIN_DPS * dt;

    room_width_px = (f32)(room->width_tiles * TILE_SIZE);
    room_height_px = (f32)(room->height_tiles * TILE_SIZE);
    if (sword->pos.x < 0.0f || sword->pos.x >= room_width_px || sword->pos.y < 0.0f || sword->pos.y >= room_height_px) {
        sword->active = false;
        return;
    }

    tx = (i32)(sword->pos.x / TILE_SIZE);
    ty = (i32)(sword->pos.y / TILE_SIZE);
    tile = room_get_tile(room, tx, ty);

    if (tile == TILE_SOLID) {
        sword->active = false;
        return;
    }

    if (tile == TILE_PLATFORM && sword->vel.y >= 0.0f) {
        f32 tile_top = (f32)(ty * TILE_SIZE);
        if (sword->pos.y <= tile_top + THROW_PROJECTILE_HEIGHT) {
            sword->active = false;
        }
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

void fighter_integrate_velocity(Fighter* fighter, f32 dt)
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