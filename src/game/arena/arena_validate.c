#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "game/arena/arena.h"
#include "arena_internal.h"

static bool tile_is_walkable(const Room *room, i32 tx, i32 ty)
{
    TileType tile;

    if (!room_is_inside(room, tx, ty))
    {
        return false;
    }

    tile = room_get_tile(room, tx, ty);
    return (tile == TILE_PLATFORM || tile == TILE_EMPTY);
}

static void accessibility_bfs(const Room *room, i32 start_tx, i32 start_ty,
                              bool visited[ROOM_HEIGHT_TILES][ROOM_MAX_WIDTH_TILES])
{
    i32 queue_head = 0;
    i32 queue_tail = 0;
    i32 queue_tx[ROOM_HEIGHT_TILES * ROOM_MAX_WIDTH_TILES];
    i32 queue_ty[ROOM_HEIGHT_TILES * ROOM_MAX_WIDTH_TILES];
    i32 max_queue = ROOM_HEIGHT_TILES * ROOM_MAX_WIDTH_TILES;

    if (!tile_is_walkable(room, start_tx, start_ty))
    {
        return;
    }

    queue_tx[queue_tail] = start_tx;
    queue_ty[queue_tail] = start_ty;
    queue_tail++;
    visited[start_ty][start_tx] = true;

    while (queue_head < queue_tail && queue_head < max_queue)
    {
        i32 tx = queue_tx[queue_head];
        i32 ty = queue_ty[queue_head];
        queue_head++;

        i32 neighbors_tx[] = {tx - 1, tx + 1, tx, tx};
        i32 neighbors_ty[] = {ty, ty, ty - 1, ty + 1};
        int i;

        for (i = 0; i < 4; ++i)
        {
            i32 nx = neighbors_tx[i];
            i32 ny = neighbors_ty[i];

            if (room_is_inside(room, nx, ny) && !visited[ny][nx] && tile_is_walkable(room, nx, ny))
            {

                visited[ny][nx] = true;
                queue_tx[queue_tail] = nx;
                queue_ty[queue_tail] = ny;
                queue_tail++;

                if (queue_tail >= max_queue)
                {
                    return;
                }
            }
        }
    }
}

bool room_validate_accessibility(const Room *room)
{
    i32 attacker_tx, attacker_ty;
    i32 defender_tx, defender_ty;
    bool visited[ROOM_HEIGHT_TILES][ROOM_MAX_WIDTH_TILES];
    bool found_attacker = false;
    bool found_defender = false;

    if (!room)
    {
        return false;
    }

    memset(visited, 0, sizeof(visited));

    attacker_tx = (i32)(room->spawns.attacker_spawn.x / TILE_SIZE);
    attacker_ty = (i32)(room->spawns.attacker_spawn.y / TILE_SIZE);

    defender_tx = (i32)(room->spawns.defender_spawn.x / TILE_SIZE);
    defender_ty = (i32)(room->spawns.defender_spawn.y / TILE_SIZE);

    accessibility_bfs(room, attacker_tx, attacker_ty, visited);

    if (room_is_inside(room, defender_tx, defender_ty))
    {
        found_defender = visited[defender_ty][defender_tx];
    }

    memset(visited, 0, sizeof(visited));
    accessibility_bfs(room, defender_tx, defender_ty, visited);

    if (room_is_inside(room, attacker_tx, attacker_ty))
    {
        found_attacker = visited[attacker_ty][attacker_tx];
    }

    return (found_attacker && found_defender);
}

bool room_validate_spawns(const Room *room)
{
    if (!room || room->width_tiles <= 0 || room->width_tiles > ROOM_MAX_WIDTH_TILES ||
        room->height_tiles != ROOM_HEIGHT_TILES)
        return false;
    Vec2 spawns[2] = {room->spawns.attacker_spawn, room->spawns.defender_spawn};
    for (i32 i = 0; i < 2; i++)
    {
        Vec2 p = spawns[i];
        if (p.x < 0 || p.y < 0 || p.x + PLAYER_WIDTH > room->width_tiles * TILE_SIZE ||
            p.y + PLAYER_HEIGHT >= room->height_tiles * TILE_SIZE)
            return false;
        i32 x0 = (i32)floorf(p.x / TILE_SIZE),
            x1 = (i32)floorf((p.x + PLAYER_WIDTH - 0.01f) / TILE_SIZE);
        i32 y0 = (i32)floorf(p.y / TILE_SIZE),
            y1 = (i32)floorf((p.y + PLAYER_HEIGHT - 0.01f) / TILE_SIZE);
        for (i32 y = y0; y <= y1; y++)
            for (i32 x = x0; x <= x1; x++)
            {
                TileType tile = room_get_tile(room, x, y);
                if (tile == TILE_SOLID || tile == TILE_HAZARD)
                    return false;
            }
        i32 feet = (i32)floorf((p.y + PLAYER_HEIGHT + 0.01f) / TILE_SIZE);
        TileType support = room_get_tile(room, x0, feet);
        if (support != TILE_SOLID && support != TILE_PLATFORM)
            return false;
    }
    return fabsf(spawns[0].x - spawns[1].x) >= PLAYER_WIDTH ||
           fabsf(spawns[0].y - spawns[1].y) >= PLAYER_HEIGHT;
}

bool room_validate_complete(const Room *room)
{
    if (!room)
    {
        return false;
    }

    if (!room_validate_spawns(room))
    {
        return false;
    }

    if (!room_validate_accessibility(room))
    {
        return false;
    }

    return true;
}
