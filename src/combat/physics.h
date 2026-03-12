#ifndef COMBAT_PHYSICS_H
#define COMBAT_PHYSICS_H

#include "types.h"

/**
 * Physics system for arena interaction
 * 
 * Handles:
 * - Collision with arena boundaries and platforms
 * - Ground detection for jumping
 * - Tile-based collision with procedurally generated arenas
 */

/* Collision detection */
bool physics_check_ground(const Arena *arena, const FighterState *fighter);
void physics_resolve_collision(const Arena *arena, FighterState *fighter);

/* Arena bounds */
void physics_clamp_to_arena(const Arena *arena, FighterState *fighter);

#endif