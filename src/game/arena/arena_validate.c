#include <string.h>
#include <stdbool.h>

#include "game/arena/arena.h"
#include "arena_internal.h"

/* =========================
   Accessibility validation
   (BFS flood-fill from spawns)
   ========================= */

static bool visited[ROOM_HEIGHT_TILES][ROOM_MAX_WIDTH_TILES];

static bool tile_is_walkable(const Room* room, i32 tx, i32 ty)
{
    TileType tile;
    
    if (!room_is_inside(room, tx, ty)) {
        return false;
    }
    
    tile = room_get_tile(room, tx, ty);
    return (tile == TILE_PLATFORM || tile == TILE_EMPTY);
}

static void accessibility_bfs(const Room* room, i32 start_tx, i32 start_ty)
{
    i32 queue_head = 0;
    i32 queue_tail = 0;
    i32 queue_tx[ROOM_HEIGHT_TILES * ROOM_MAX_WIDTH_TILES];
    i32 queue_ty[ROOM_HEIGHT_TILES * ROOM_MAX_WIDTH_TILES];
    i32 max_queue = ROOM_HEIGHT_TILES * ROOM_MAX_WIDTH_TILES;
    
    if (!tile_is_walkable(room, start_tx, start_ty)) {
        return;
    }
    
    queue_tx[queue_tail] = start_tx;
    queue_ty[queue_tail] = start_ty;
    queue_tail++;
    visited[start_ty][start_tx] = true;
    
    while (queue_head < queue_tail && queue_head < max_queue) {
        i32 tx = queue_tx[queue_head];
        i32 ty = queue_ty[queue_head];
        queue_head++;
        
        /* Check 4 neighbors: left, right, up, down */
        i32 neighbors_tx[] = {tx - 1, tx + 1, tx, tx};
        i32 neighbors_ty[] = {ty, ty, ty - 1, ty + 1};
        int i;
        
        for (i = 0; i < 4; ++i) {
            i32 nx = neighbors_tx[i];
            i32 ny = neighbors_ty[i];
            
            if (room_is_inside(room, nx, ny) && 
                !visited[ny][nx] && 
                tile_is_walkable(room, nx, ny)) {
                
                visited[ny][nx] = true;
                queue_tx[queue_tail] = nx;
                queue_ty[queue_tail] = ny;
                queue_tail++;
                
                if (queue_tail >= max_queue) {
                    return; /* Queue overflow, stop */
                }
            }
        }
    }
}

bool room_validate_accessibility(const Room* room)
{
    i32 attacker_tx, attacker_ty;
    i32 defender_tx, defender_ty;
    bool found_attacker = false;
    bool found_defender = false;
    
    if (!room) {
        return false;
    }
    
    /* Clear visited array */
    memset(visited, 0, sizeof(visited));
    
    /* Convert spawn positions to tile coordinates */
    attacker_tx = (i32)(room->spawns.attacker_spawn.x / TILE_SIZE);
    attacker_ty = (i32)(room->spawns.attacker_spawn.y / TILE_SIZE);
    
    defender_tx = (i32)(room->spawns.defender_spawn.x / TILE_SIZE);
    defender_ty = (i32)(room->spawns.defender_spawn.y / TILE_SIZE);
    
    /* BFS from attacker spawn */
    accessibility_bfs(room, attacker_tx, attacker_ty);
    
    /* Check if defender spawn reached */
    if (room_is_inside(room, defender_tx, defender_ty)) {
        found_defender = visited[defender_ty][defender_tx];
    }
    
    /* BFS from defender spawn (to ensure bidirectional) */
    memset(visited, 0, sizeof(visited));
    accessibility_bfs(room, defender_tx, defender_ty);
    
    if (room_is_inside(room, attacker_tx, attacker_ty)) {
        found_attacker = visited[attacker_ty][attacker_tx];
    }
    
    return (found_attacker && found_defender);
}

/* =========================
   Spawn validation
   ========================= */

bool room_validate_spawns(const Room* room)
{
    i32 attacker_tx, attacker_ty;
    i32 defender_tx, defender_ty;
    TileType attacker_tile, defender_tile;
    
    if (!room) {
        return false;
    }
    
    /* Convert spawn positions to tile coordinates */
    attacker_tx = (i32)(room->spawns.attacker_spawn.x / TILE_SIZE);
    attacker_ty = (i32)(room->spawns.attacker_spawn.y / TILE_SIZE);
    
    defender_tx = (i32)(room->spawns.defender_spawn.x / TILE_SIZE);
    defender_ty = (i32)(room->spawns.defender_spawn.y / TILE_SIZE);
    
    /* Check bounds */
    if (!room_is_inside(room, attacker_tx, attacker_ty) ||
        !room_is_inside(room, defender_tx, defender_ty)) {
        return false;
    }
    
    /* Spawns must not be on hazards */
    attacker_tile = room_get_tile(room, attacker_tx, attacker_ty);
    defender_tile = room_get_tile(room, defender_tx, defender_ty);
    
    if (attacker_tile == TILE_HAZARD || defender_tile == TILE_HAZARD) {
        return false;
    }
    
    /* Spawns should not overlap */
    if (attacker_tx == defender_tx && attacker_ty == defender_ty) {
        return false;
    }
    
    /* Spawns must have floor support (or be on platform) */
    if (room_get_tile(room, attacker_tx, attacker_ty + 1) == TILE_EMPTY) {
        return false;
    }
    if (room_get_tile(room, defender_tx, defender_ty + 1) == TILE_EMPTY) {
        return false;
    }
    
    return true;
}

/* =========================
   Complete validation
   ========================= */

bool room_validate_complete(const Room* room)
{
    if (!room) {
        return false;
    }
    
    /* Check spawns first (fast) */
    if (!room_validate_spawns(room)) {
        return false;
    }
    
    /* Then check accessibility (slower) */
    if (!room_validate_accessibility(room)) {
        return false;
    }
    
    return true;
}
