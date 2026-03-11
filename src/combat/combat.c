#include "combat.h"
#include "arena.h"
#include "constants.h"

#define MOVE_SPEED    4.0f
#define JUMP_SPEED  -13.0f
#define ATTACK_RANGE 110.0f
#define PARRY_FRAMES  20
#define GRAVITY       0.55f
#define MAX_FALL      12.0f


GameError combat_init(CombatState *combat, const Arena *arena, const GameConfig *config) {
	(void)config;
	if (combat == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	*combat = (CombatState){0};
	combat->duel_active = true;

	/* Floor y in pixels */
	int floor_tile_y = (arena != NULL && arena->height > 0) ? (int)(arena->height - 1) : 11;
	float fighter_floor_y = (float)(floor_tile_y * ARENA_TILE_H - FIGHTER_H);

	/* Default spawn columns */
	Vec2i spawn1 = {2, floor_tile_y - 1};
	Vec2i spawn2 = {(arena != NULL && arena->width > 3) ? (int32_t)(arena->width - 3) : 13,
	                floor_tile_y - 1};

	if (arena != NULL) {
		arena_find_spawn(arena, PLAYER_ONE, &spawn1);
		arena_find_spawn(arena, PLAYER_TWO, &spawn2);
	}

	float p1_x = (float)(spawn1.x * ARENA_TILE_W + (ARENA_TILE_W - FIGHTER_W) / 2);
	float p2_x = (float)(spawn2.x * ARENA_TILE_W + (ARENA_TILE_W - FIGHTER_W) / 2);

	combat->fighters[PLAYER_ONE] = (FighterState){
		.position     = {p1_x, fighter_floor_y},
		.facing       = FACING_RIGHT,
		.sword_height = SWORD_HEIGHT_MID,
		.alive        = true,
		.grounded     = true,
	};
	combat->fighters[PLAYER_TWO] = (FighterState){
		.position     = {p2_x, fighter_floor_y},
		.facing       = FACING_LEFT,
		.sword_height = SWORD_HEIGHT_MID,
		.alive        = true,
		.grounded     = true,
	};

	return GAME_OK;
}

GameError combat_apply_action(CombatState *combat, PlayerId actor, const Action *action) {
	if (combat == NULL || action == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	FighterState *self  = &combat->fighters[actor];
	FighterState *other = &combat->fighters[actor == PLAYER_ONE ? PLAYER_TWO : PLAYER_ONE];

	if (!self->alive) return GAME_OK;

	switch (action->type) {
	case ACTION_MOVE_LEFT:
		self->velocity.x = -MOVE_SPEED;
		self->facing = FACING_LEFT;
		break;
	case ACTION_MOVE_RIGHT:
		self->velocity.x = MOVE_SPEED;
		self->facing = FACING_RIGHT;
		break;
	case ACTION_JUMP:
		if (self->grounded) {
			self->velocity.y = JUMP_SPEED;
			self->grounded   = false;
		}
		break;
	case ACTION_HEIGHT_CHANGE:
		self->sword_height = action->sword_height;
		break;
	case ACTION_PARRY:
		self->stun_frames  = PARRY_FRAMES;
		self->sword_height = action->sword_height;
		break;
	case ACTION_ATTACK: {
		if (!other->alive) break;
		self->sword_height = action->sword_height;
		float dx   = other->position.x - self->position.x;
		float dist = (dx < 0.0f) ? -dx : dx;
		if (dist <= ATTACK_RANGE) {
			if (other->stun_frames > 0 &&
			    other->sword_height == action->sword_height) {
				/* Parry success: stun attacker */
				self->stun_frames = 30;
				other->successful_parries++;
				other->stun_frames = 0;
			} else {
				other->alive    = false;
				other->velocity = (Vec2f){0.0f, 0.0f};
			}
		}
		break;
	}
	default:
		break;
	}

	return GAME_OK;
}

GameError combat_step(CombatState *combat, const Arena *arena, uint32_t fixed_dt_ms) {
	(void)fixed_dt_ms;
	if (combat == NULL) return GAME_ERROR_INVALID_ARGUMENT;

	int arena_w_px = 1280;
	float fighter_floor_y = (float)(11 * ARENA_TILE_H - FIGHTER_H);
	if (arena != NULL && arena->width > 0 && arena->height > 0) {
		arena_w_px       = (int)(arena->width  * ARENA_TILE_W);
		fighter_floor_y  = (float)((arena->height - 1) * ARENA_TILE_H - FIGHTER_H);
	}

	for (int i = 0; i < 2; i++) {
		FighterState *f = &combat->fighters[i];
		if (!f->alive) continue;

		/* Gravity */
		if (!f->grounded) {
			f->velocity.y += GRAVITY;
			if (f->velocity.y > MAX_FALL) f->velocity.y = MAX_FALL;
		}

		/* Integrate position */
		f->position.x += f->velocity.x;
		f->position.y += f->velocity.y;

		/* Floor collision */
		if (f->position.y >= fighter_floor_y) {
			f->position.y = fighter_floor_y;
			f->velocity.y = 0.0f;
			f->grounded   = true;
		} else {
			f->grounded = false;
		}

		/* Horizontal bounds */
		if (f->position.x < 0.0f) {
			f->position.x = 0.0f;
			f->velocity.x = 0.0f;
		}
		float max_x = (float)(arena_w_px - FIGHTER_W);
		if (f->position.x > max_x) {
			f->position.x = max_x;
			f->velocity.x = 0.0f;
		}

		/* Friction: instant stop */
		f->velocity.x = 0.0f;

		/* Decay parry window */
		if (f->stun_frames > 0) f->stun_frames--;
	}

	combat->round_time_frames++;
	return GAME_OK;
}

GameError combat_reset_round(CombatState *combat, const Arena *arena, const GameConfig *config) {
	if (combat == NULL) return GAME_ERROR_INVALID_ARGUMENT;
	uint8_t saved[2] = {combat->score[0], combat->score[1]};
	GameError err = combat_init(combat, arena, config);
	if (err != GAME_OK) return err;
	combat->score[0] = saved[0];
	combat->score[1] = saved[1];
	return GAME_OK;
}

bool combat_is_round_over(const CombatState *combat, PlayerId *out_winner) {
	if (combat == NULL || !combat->duel_active) return false;

	bool p1 = combat->fighters[PLAYER_ONE].alive;
	bool p2 = combat->fighters[PLAYER_TWO].alive;

	if (p1 && p2) return false;

	if (out_winner)
		*out_winner = p1 ? PLAYER_ONE : PLAYER_TWO;
	return true;
}

