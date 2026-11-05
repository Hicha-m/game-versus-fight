#include "combat_internal.h"
#include "utils/utils.h"
#include <math.h>

static void update_projectile(CombatState *combat, const Room *room, ThrownSword *sword, f32 dt)
{
    if (!combat || !room || dt <= 0)
        return;
    if (!sword->active)
        return;
    sword->age += dt;
    /* A thrown rapier travels level until contact, as in the reference game. */

    sword->rotation_deg += (sword->vel.x >= 0 ? 1 : -1) * THROW_PROJECTILE_SPIN_DPS * dt;
    i32 steps = (i32)ceilf(fmaxf(fabsf(sword->vel.x * dt), fabsf(sword->vel.y * dt)) / 4);
    if (steps < 1)
        steps = 1;
    for (i32 step = 0; step < steps; step++)
    {
        Vec2 before = sword->pos;
        sword->pos.x += sword->vel.x * dt / steps;
        sword->pos.y += sword->vel.y * dt / steps;
        i32 tx = (i32)floorf(sword->pos.x / TILE_SIZE), ty = (i32)floorf(sword->pos.y / TILE_SIZE);
        TileType tile = room_get_tile(room, tx, ty);
        bool outside =
            sword->pos.x < 0 || sword->pos.x >= room->width_tiles * TILE_SIZE || sword->pos.y < 0;
        bool platform = tile == TILE_PLATFORM && sword->vel.y >= 0 && before.y <= ty * TILE_SIZE;
        if (tile == TILE_HAZARD || sword->pos.y >= room->height_tiles * TILE_SIZE)
        {
            sword->active = false;
            return;
        }
        if (tile == TILE_SOLID || platform || outside)
        {
            Vec2 dropped = {clampf(before.x, 0, room->width_tiles * TILE_SIZE - PLAYER_WIDTH),
                            fmaxf(0, before.y) - PLAYER_HEIGHT};
            combat_drop_sword(combat, dropped);
            sword->active = false;
            return;
        }
    }
}

void combat_update_thrown_sword_physics(CombatState *combat, const Room *room, f32 dt)
{
    if (!combat)
        return;
    update_projectile(combat, room, &combat->thrown_sword, dt);
    update_projectile(combat, room, &combat->second_thrown_sword, dt);
}

void fighter_apply_gravity(Fighter *fighter, f32 dt)
{
    if (!fighter->state.alive)
    {
        return;
    }

    if (!fighter->state.grounded)
    {
        fighter->state.vel.y += fighter->stats.gravity * dt;
        fighter->state.vel.y =
            clampf(fighter->state.vel.y, -10000.0f, fighter->stats.max_fall_speed);

        if (!fighter->state.is_attacking && !fighter->state.is_stunned &&
            fighter->state.action_duration <= 0)
        {
            fighter->state.action =
                (fighter->state.vel.y < 0.0f) ? FIGHTER_ACTION_JUMP : FIGHTER_ACTION_FALL;
        }
    }
}

void fighter_integrate_velocity(Fighter *fighter, f32 dt)
{
    if (!fighter->state.alive)
    {
        return;
    }

    fighter->state.pos.x += fighter->state.vel.x * dt;
    fighter->state.pos.y += fighter->state.vel.y * dt;
}

static bool body_hits_shape(const Room *room, f32 x, f32 y, f32 height)
{
    i32 x0 = (i32)floorf(x / TILE_SIZE), x1 = (i32)floorf((x + PLAYER_WIDTH - 0.01f) / TILE_SIZE);
    i32 y0 = (i32)floorf(y / TILE_SIZE), y1 = (i32)floorf((y + height - 0.01f) / TILE_SIZE);
    for (i32 ty = y0; ty <= y1; ty++)
        for (i32 tx = x0; tx <= x1; tx++)
            if (room_get_tile(room, tx, ty) == TILE_SOLID)
                return true;
    return false;
}

static bool body_hits(const Room *room, f32 x, f32 y)
{
    return body_hits_shape(room, x, y, PLAYER_HEIGHT);
}

void fighter_resolve_world_collision(Fighter *f, const Room *room, f32 dt)
{
    if (!f || !room || !f->state.alive || dt <= 0)
        return;
    f32 dx = f->state.vel.x * dt, dy = f->state.vel.y * dt;
    f32 x = f->state.pos.x - dx, y = f->state.pos.y - dy;
    if (body_hits(room, x, y) &&
        !body_hits_shape(room, x, y + PLAYER_HEIGHT / 2, PLAYER_HEIGHT / 2))
        f->state.crouching = true;
    f32 offset = fighter_get_body_rect(f).y - f->state.pos.y;
    f32 height = PLAYER_HEIGHT - offset;
    f->state.wall_direction = 0;
    i32 steps = (i32)ceilf(fmaxf(fabsf(dx), fabsf(dy)) / 4.0f);
    if (steps < 1)
        steps = 1;
    bool grounded = false;
    for (i32 i = 0; i < steps; i++)
    {
        f32 nx = x + dx / steps;
        if (!body_hits_shape(room, nx, y + offset, height))
            x = nx;
        else
        {
            f->state.wall_direction = dx > 0 ? 1 : -1;
            dx = 0;
            f->state.vel.x = 0;
        }
        f32 ny = y + dy / steps;
        bool platform = false;
        if (dy >= 0)
        {
            i32 row = (i32)floorf((ny + PLAYER_HEIGHT + 0.01f) / TILE_SIZE);
            for (i32 tx = (i32)floorf(x / TILE_SIZE);
                 tx <= (i32)floorf((x + PLAYER_WIDTH - 0.01f) / TILE_SIZE); tx++)
            {
                TileType t = room_get_tile(room, tx, row);
                if ((t == TILE_PLATFORM || t == TILE_CLOUD || t == TILE_SOLID) &&
                    y + PLAYER_HEIGHT <= row * TILE_SIZE + 0.01f &&
                    ny + PLAYER_HEIGHT >= row * TILE_SIZE)
                {
                    ny = row * TILE_SIZE - PLAYER_HEIGHT;
                    platform = true;
                }
            }
        }
        if (platform || body_hits_shape(room, x, ny + offset, height))
        {
            if (dy >= 0)
                grounded = true;
            if (platform)
                y = ny;
            dy = 0;
            f->state.vel.y = 0;
        }
        else
            y = ny;
    }
    f->state.pos.x = clampf(x, 0, room->width_tiles * TILE_SIZE - PLAYER_WIDTH);
    f->state.pos.y = y;
    f->state.grounded = grounded;
    if (!grounded && f->state.wall_direction && f->state.wall_jump_lock <= 0)
        f->state.vel.y = f->state.climbing ? -140.0f : fminf(f->state.vel.y, 120.0f);
    if (grounded)
        f->state.coyote_time = DEFAULT_COYOTE_TIME;
    for (i32 ty = (i32)floorf(y / TILE_SIZE);
         ty <= (i32)floorf((y + PLAYER_HEIGHT - 0.01f) / TILE_SIZE); ty++)
        for (i32 tx = (i32)floorf(x / TILE_SIZE);
             tx <= (i32)floorf((x + PLAYER_WIDTH - 0.01f) / TILE_SIZE); tx++)
            if (room_get_tile(room, tx, ty) == TILE_HAZARD)
            {
                f->state.alive = false;
                f->state.action = FIGHTER_ACTION_DEAD;
            }
}

/* Conservative certificate: actually traverse with the production motion solver.
 * Failure means regenerate/fallback, not proof that no human route exists. */
bool combat_room_traversable(const Room *room)
{
    if (!room || room->width_tiles < 2 || room->width_tiles > ROOM_MAX_WIDTH_TILES ||
        room->height_tiles != ROOM_HEIGHT_TILES)
        return false;
    for (i32 direction = -1; direction <= 1; direction += 2)
    {
        Fighter f = {0};
        f.state.alive = true;
        f.state.grounded = true;
        f.stats = (FighterStats){PLAYER_MOVE_SPEED, PLAYER_JUMP_SPEED, PLAYER_GRAVITY,
                                 PLAYER_MAX_FALL_SPEED};
        if (!combat_safe_spawn(room,
                               direction > 0 ? 0 : room->width_tiles * TILE_SIZE - PLAYER_WIDTH,
                               &f.state.pos))
            return false;
        if (body_hits(room, f.state.pos.x, f.state.pos.y))
            return false;
        bool reached = false;
        for (i32 tick = 0; tick < 1800; tick++)
        {
            f32 ahead = f.state.pos.x + PLAYER_WIDTH / 2 + direction * 48;
            i32 tx = (i32)floorf(ahead / TILE_SIZE);
            i32 floor = (i32)floorf((f.state.pos.y + PLAYER_HEIGHT + 4) / TILE_SIZE);
            TileType below = room_get_tile(room, tx, floor);
            bool obstacle = body_hits(room, f.state.pos.x + direction * 20, f.state.pos.y);
            PlayerCommand cmd = {0};
            cmd.right_pressed = direction > 0;
            cmd.left_pressed = direction < 0;
            cmd.jump_pressed =
                f.state.grounded && (obstacle || below == TILE_EMPTY || below == TILE_HAZARD);
            fighter_update_timers(&f, FIXED_DT);
            fighter_apply_command(&f, &cmd, FIXED_DT);
            fighter_apply_gravity(&f, FIXED_DT);
            fighter_integrate_velocity(&f, FIXED_DT);
            fighter_resolve_world_collision(&f, room, FIXED_DT);
            if (!f.state.alive || f.state.pos.y > room->height_tiles * TILE_SIZE)
                return false;
            if (direction > 0 ? f.state.pos.x + PLAYER_WIDTH >= room->transition.right_trigger_x
                              : f.state.pos.x <= room->transition.left_trigger_x)
            {
                reached = true;
                break;
            }
        }
        if (!reached)
            return false;
    }
    return true;
}

bool combat_safe_spawn(const Room *room, f32 desired_x, Vec2 *out)
{
    if (!room || !out)
        return false;
    i32 center = (i32)(desired_x / TILE_SIZE);
    for (i32 offset = 0; offset < room->width_tiles; offset++)
        for (i32 sign = -1; sign <= 1; sign += 2)
        {
            i32 x = center + sign * offset;
            if (x < 0 || x >= room->width_tiles)
                continue;
            for (i32 row = room->height_tiles - 1; row >= 2; row--)
            {
                TileType floor = room_get_tile(room, x, row);
                Vec2 pos = {(f32)(x * TILE_SIZE), (f32)(row * TILE_SIZE) - PLAYER_HEIGHT};
                if ((floor == TILE_SOLID || floor == TILE_PLATFORM || floor == TILE_CLOUD) &&
                    !body_hits(room, pos.x, pos.y))
                {
                    bool hazard = false;
                    for (i32 y = row - 2; y < row; y++)
                        if (room_get_tile(room, x, y) == TILE_HAZARD)
                            hazard = true;
                    if (!hazard)
                    {
                        *out = pos;
                        return true;
                    }
                }
            }
        }
    return false;
}
