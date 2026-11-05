#include "combat_internal.h"
#include "utils/utils.h"
#include <math.h>

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
        for (i32 i=0;i<MAX_PLAYERS;i++) if(!combat->ground_swords[i].active) {
            combat->ground_swords[i].active=true;
            combat->ground_swords[i].pos=(Vec2){clampf(sword->pos.x,0,room->width_tiles*TILE_SIZE-PLAYER_WIDTH), sword->pos.y};
            break;
        }
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
        for (i32 i=0;i<MAX_PLAYERS;i++) if(!combat->ground_swords[i].active) {
            combat->ground_swords[i].active=true;
            combat->ground_swords[i].pos=(Vec2){clampf(sword->pos.x,0,room->width_tiles*TILE_SIZE-PLAYER_WIDTH), sword->pos.y};
            break;
        }
        sword->active = false;
        return;
    }

    tx = (i32)(sword->pos.x / TILE_SIZE);
    ty = (i32)(sword->pos.y / TILE_SIZE);
    tile = room_get_tile(room, tx, ty);

    if (tile == TILE_SOLID) {
        for (i32 i=0;i<MAX_PLAYERS;i++) if(!combat->ground_swords[i].active) {
            combat->ground_swords[i].active=true;
            combat->ground_swords[i].pos=(Vec2){clampf(sword->pos.x,0,room->width_tiles*TILE_SIZE-PLAYER_WIDTH), sword->pos.y};
            break;
        }
        sword->active = false;
        return;
    }

    if (tile == TILE_PLATFORM && sword->vel.y >= 0.0f) {
        f32 tile_top = (f32)(ty * TILE_SIZE);
        if (sword->pos.y <= tile_top + THROW_PROJECTILE_HEIGHT) {
            for (i32 i=0;i<MAX_PLAYERS;i++) if(!combat->ground_swords[i].active) {
            combat->ground_swords[i].active=true;
            combat->ground_swords[i].pos=(Vec2){clampf(sword->pos.x,0,room->width_tiles*TILE_SIZE-PLAYER_WIDTH), sword->pos.y};
            break;
        }
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

static bool body_hits(const Room* room, f32 x, f32 y)
{
    i32 x0=(i32)floorf(x/TILE_SIZE), x1=(i32)floorf((x+PLAYER_WIDTH-0.01f)/TILE_SIZE);
    i32 y0=(i32)floorf(y/TILE_SIZE), y1=(i32)floorf((y+PLAYER_HEIGHT-0.01f)/TILE_SIZE);
    for(i32 ty=y0;ty<=y1;ty++) for(i32 tx=x0;tx<=x1;tx++)
        if(room_get_tile(room,tx,ty)==TILE_SOLID) return true;
    return false;
}

void fighter_resolve_world_collision(Fighter* f, const Room* room, f32 dt)
{
    if(!f || !room || !f->state.alive || dt<=0) return;
    f32 dx=f->state.vel.x*dt, dy=f->state.vel.y*dt;
    f32 x=f->state.pos.x-dx, y=f->state.pos.y-dy;
    i32 steps=(i32)ceilf(fmaxf(fabsf(dx),fabsf(dy))/4.0f); if(steps<1) steps=1;
    bool grounded=false;
    for(i32 i=0;i<steps;i++) {
        f32 nx=x+dx/steps;
        if(!body_hits(room,nx,y)) x=nx; else {dx=0;f->state.vel.x=0;}
        f32 ny=y+dy/steps;
        bool platform=false;
        if(dy>=0) {
            i32 row=(i32)floorf((ny+PLAYER_HEIGHT+0.01f)/TILE_SIZE);
            for(i32 tx=(i32)floorf(x/TILE_SIZE);tx<=(i32)floorf((x+PLAYER_WIDTH-0.01f)/TILE_SIZE);tx++) {
                TileType t=room_get_tile(room,tx,row);
                if((t==TILE_PLATFORM || t==TILE_SOLID) && y+PLAYER_HEIGHT<=row*TILE_SIZE+0.01f && ny+PLAYER_HEIGHT>=row*TILE_SIZE) {
                    ny=row*TILE_SIZE-PLAYER_HEIGHT;platform=true;
                }
            }
        }
        if(platform || body_hits(room,x,ny)) {
            if(dy>=0) grounded=true;
            if(platform) y=ny;
            dy=0;f->state.vel.y=0;
        } else y=ny;
    }
    f->state.pos.x=clampf(x,0,room->width_tiles*TILE_SIZE-PLAYER_WIDTH);
    f->state.pos.y=y;
    f->state.grounded=grounded;
    if(grounded) f->state.coyote_time=DEFAULT_COYOTE_TIME;
    i32 tx=(i32)floorf((x+PLAYER_WIDTH/2)/TILE_SIZE),ty=(i32)floorf((y+PLAYER_HEIGHT/2)/TILE_SIZE);
    if(room_get_tile(room,tx,ty)==TILE_HAZARD) {f->state.alive=false;f->state.action=FIGHTER_ACTION_DEAD;}
}
