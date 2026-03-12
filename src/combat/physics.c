#include "combat/physics.h"
#include "constants.h"

/**
 * Check if a fighter is standing on ground
 * Simple raycast: check if tile below fighter is solid
 */
bool physics_check_ground(const Arena *arena, const FighterState *fighter) {
	if (arena == NULL || fighter == NULL) return false;
	
	// Check if there's solid ground below fighter
	int grid_x = (int)fighter->position.x;
	int grid_y = (int)(fighter->position.y + PLAYER_HEIGHT) + 1;  // One unit below feet
	
	// Out of bounds = solid (arena boundary acts as ground)
	if (grid_y >= (int)arena->height) return true;
	if (grid_y < 0 || grid_x < 0 || grid_x >= (int)arena->width) return false;
	
	// Check tile type
	if (arena->tiles == NULL) return false;
	int tile_index = grid_y * arena->width + grid_x;
	if (tile_index >= 0 && tile_index < (int)(arena->width * arena->height)) {
		TileType tile = arena->tiles[tile_index];
		return tile == TILE_SOLID || tile == TILE_PLATFORM;
	}
	
	return false;
}

/**
 * Simple AABB collision resolution
 * Prevents fighters from going through walls and floors
 */
void physics_resolve_collision(const Arena *arena, FighterState *fighter) {
	if (arena == NULL || fighter == NULL) return;
	
	// Check ground contact
	fighter->grounded = physics_check_ground(arena, fighter);
	
	// Prevent falling through bottom
	if (fighter->position.y > (float)arena->height) {
		fighter->position.y = (float)arena->height;
		fighter->velocity.y = 0;
		fighter->grounded = true;
	}
	
	// Crude collision with solid tiles (prevent clipping)
	if (arena->tiles == NULL) return;
	for (int dy = 0; dy < 3; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			int grid_x = (int)(fighter->position.x + dx);
			int grid_y = (int)(fighter->position.y + dy);
			
			if (grid_x < 0 || grid_x >= (int)arena->width ||
				grid_y < 0 || grid_y >= (int)arena->height) continue;
			
			int idx = grid_y * arena->width + grid_x;
			if (idx >= 0 && idx < (int)(arena->width * arena->height)) {
				TileType tile = arena->tiles[idx];
				if (tile == TILE_SOLID) {
					// Push fighter out of solid tile
					if (dy == 2) {
						// Coming from above (floor)
						fighter->position.y = grid_y - PLAYER_HEIGHT;
						fighter->velocity.y = 0;
						fighter->grounded = true;
					} else {
						// Side collision - push out
						if (dx > 0) fighter->position.x = grid_x - PLAYER_WIDTH;
						else if (dx < 0) fighter->position.x = grid_x + 1;
					}
				}
			}
		}
	}
}

/**
 * Clamp fighter to arena bounds
 * Prevents fighters from leaving the level horizontally
 */
void physics_clamp_to_arena(const Arena *arena, FighterState *fighter) {
	if (arena == NULL || fighter == NULL) return;
	
	if (fighter->position.x < 0) {
		fighter->position.x = 0;
		fighter->velocity.x = 0;
	}
	
	if (fighter->position.x + PLAYER_WIDTH > (float)arena->width) {
		fighter->position.x = (float)arena->width - PLAYER_WIDTH;
		fighter->velocity.x = 0;
	}
}
