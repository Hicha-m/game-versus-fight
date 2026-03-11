#include "state.h"
#include "constants.h"

#include <stddef.h>

static bool ai_is_valid_player(PlayerId actor) {
	return actor == PLAYER_ONE || actor == PLAYER_TWO;
}

static int32_t ai_pixel_to_tile_x(float px) {
	return (int32_t)(px / (float)ARENA_TILE_W);
}

static int32_t ai_pixel_to_tile_y(float py) {
	return (int32_t)(py / (float)ARENA_TILE_H);
}

static bool ai_is_inside_arena(const Arena *arena, int32_t x, int32_t y) {
	return x >= 0 && y >= 0 && x < (int32_t)arena->width && y < (int32_t)arena->height;
}

static TileType ai_tile_at(const Arena *arena, int32_t x, int32_t y) {
	const size_t idx = (size_t)y * (size_t)arena->width + (size_t)x;
	return arena->tiles[idx];
}

static AiLocalZone ai_zone_from_tile(TileType tile) {
	if (tile == TILE_HAZARD) {
		return AI_LOCAL_ZONE_HAZARD;
	}
	return AI_LOCAL_ZONE_NEUTRAL;
}

static AiLocalObstacle ai_obstacle_between(const Arena *arena, int32_t x0, int32_t x1, int32_t y) {
	int32_t left = x0;
	int32_t right = x1;
	int32_t x = 0;

	if (left > right) {
		left = x1;
		right = x0;
	}

	for (x = left + 1; x < right; ++x) {
		const TileType tile = ai_tile_at(arena, x, y);
		if (tile == TILE_HAZARD) {
			return AI_LOCAL_OBSTACLE_HAZARD;
		}
		if (tile == TILE_SOLID || tile == TILE_PLATFORM) {
			return AI_LOCAL_OBSTACLE_SOLID;
		}
	}

	return AI_LOCAL_OBSTACLE_NONE;
}

static AiLocalDanger ai_compute_danger(const FighterState *actor, const FighterState *opponent,
		int32_t dx, int32_t dy) {
	const int32_t abs_dx = dx >= 0 ? dx : -dx;
	const int32_t abs_dy = dy >= 0 ? dy : -dy;

	if (!opponent->alive) {
		return AI_LOCAL_DANGER_NONE;
	}

	if (abs_dx <= 1 && abs_dy <= 1) {
		if (opponent->grounded && opponent->sword_height == actor->sword_height) {
			return AI_LOCAL_DANGER_HIGH;
		}
		return AI_LOCAL_DANGER_LOW;
	}

	if (abs_dx <= 3 && abs_dy <= 1) {
		return AI_LOCAL_DANGER_LOW;
	}

	return AI_LOCAL_DANGER_NONE;
}

GameError ai_state_discretize(const GameState *state, PlayerId actor, AiDiscreteState *out_state) {
	PlayerId opponent = PLAYER_ONE;
	const FighterState *actor_fighter = NULL;
	const FighterState *opponent_fighter = NULL;
	const Arena *arena = NULL;
	int32_t actor_x = 0;
	int32_t actor_y = 0;
	int32_t opp_x = 0;
	int32_t opp_y = 0;

	if (state == NULL || out_state == NULL || !ai_is_valid_player(actor)) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	arena = &state->arena;
	if (arena->tiles == NULL || arena->width == 0U || arena->height == 0U) {
		return GAME_ERROR_INVALID_STATE;
	}

	opponent = actor == PLAYER_ONE ? PLAYER_TWO : PLAYER_ONE;
	actor_fighter = &state->combat.fighters[actor];
	opponent_fighter = &state->combat.fighters[opponent];

	actor_x = ai_pixel_to_tile_x(actor_fighter->position.x);
	actor_y = ai_pixel_to_tile_y(actor_fighter->position.y);
	opp_x   = ai_pixel_to_tile_x(opponent_fighter->position.x);
	opp_y   = ai_pixel_to_tile_y(opponent_fighter->position.y);

	if (!ai_is_inside_arena(arena, actor_x, actor_y) || !ai_is_inside_arena(arena, opp_x, opp_y)) {
		return GAME_ERROR_OUT_OF_BOUNDS;
	}

	out_state->actor_alive = actor_fighter->alive ? 1U : 0U;
	out_state->actor_grounded = actor_fighter->grounded ? 1U : 0U;
	out_state->actor_sword_height = actor_fighter->sword_height;
	out_state->opponent_distance_x = (int16_t)(opp_x - actor_x);
	out_state->opponent_distance_y = (int16_t)(opp_y - actor_y);
	out_state->danger = ai_compute_danger(actor_fighter, opponent_fighter,
		out_state->opponent_distance_x, out_state->opponent_distance_y);
	out_state->obstacle_between = ai_obstacle_between(arena, actor_x, opp_x, actor_y);
	out_state->local_zone = ai_zone_from_tile(ai_tile_at(arena, actor_x, actor_y));

	return GAME_OK;
}
