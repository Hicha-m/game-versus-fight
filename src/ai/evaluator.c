#include "evaluator.h"
#include "constants.h"

#include <stddef.h>

static bool ai_eval_is_valid_player(PlayerId actor) {
	return actor == PLAYER_ONE || actor == PLAYER_TWO;
}

static PlayerId ai_eval_opponent(PlayerId actor) {
	return actor == PLAYER_ONE ? PLAYER_TWO : PLAYER_ONE;
}

static int32_t ai_eval_abs(int32_t value) {
	return value >= 0 ? value : -value;
}

static int32_t ai_eval_px_to_tile_x(float px) {
	return (int32_t)(px / (float)ARENA_TILE_W);
}

static int32_t ai_eval_px_to_tile_y(float py) {
	return (int32_t)(py / (float)ARENA_TILE_H);
}

static bool ai_eval_inside_arena(const Arena *arena, int32_t x, int32_t y) {
	return x >= 0 && y >= 0 && x < (int32_t)arena->width && y < (int32_t)arena->height;
}

static TileType ai_eval_tile_at(const Arena *arena, int32_t x, int32_t y) {
	const size_t idx = (size_t)y * (size_t)arena->width + (size_t)x;
	return arena->tiles[idx];
}

static Action ai_eval_default_action(void) {
	Action action = {0};
	action.type = ACTION_NONE;
	action.sword_height = SWORD_HEIGHT_MID;
	action.issued_frame = 0U;
	return action;
}

static int ai_eval_action_priority(const Action *action) {
	switch (action->type) {
		case ACTION_PARRY:
			return 0;
		case ACTION_ATTACK:
			return 1;
		case ACTION_HEIGHT_CHANGE:
			return 2;
		case ACTION_MOVE_LEFT:
		case ACTION_MOVE_RIGHT:
			return 3;
		case ACTION_NONE:
			return 4;
		case ACTION_JUMP:
		default:
			return 5;
	}
}

static void ai_eval_insert_candidate(AiActionList *list, const Action *action, int32_t score) {
	AiCandidateAction candidate = {0};
	uint8_t insert_at = list->count;
	uint8_t idx = 0U;

	for (idx = 0U; idx < list->count; ++idx) {
		if (list->items[idx].action.type == action->type &&
				list->items[idx].action.sword_height == action->sword_height) {
			if (score > list->items[idx].score) {
				list->items[idx].score = score;
			}
			return;
		}
	}

	if (list->count >= AI_MAX_CANDIDATE_ACTIONS) {
		if (score < list->items[list->count - 1U].score) {
			return;
		}
		insert_at = list->count - 1U;
	} else {
		list->count++;
	}

	candidate.action = *action;
	candidate.score = score;

	while (insert_at > 0U) {
		AiCandidateAction previous = list->items[insert_at - 1U];
		const bool better_score = score > previous.score;
		const bool same_score_better_tiebreak = score == previous.score &&
			ai_eval_action_priority(action) < ai_eval_action_priority(&previous.action);
		if (!better_score && !same_score_better_tiebreak) {
			break;
		}
		list->items[insert_at] = previous;
		insert_at--;
	}

	list->items[insert_at] = candidate;
}

static SwordHeight ai_eval_preferred_height(const FighterState *actor_fighter, const FighterState *opponent_fighter) {
	if (actor_fighter->sword_height != opponent_fighter->sword_height) {
		return opponent_fighter->sword_height;
	}
	if (actor_fighter->sword_height == SWORD_HEIGHT_MID) {
		return SWORD_HEIGHT_HIGH;
	}
	return SWORD_HEIGHT_MID;
}

GameError ai_evaluator_score_action(const GameState *state, PlayerId actor, const Action *action, int32_t *out_score) {
	AiDiscreteState discrete = {0};
	const FighterState *actor_fighter = NULL;
	const FighterState *opponent_fighter = NULL;
	PlayerId opponent = PLAYER_ONE;
	int32_t score = 0;
	int32_t abs_dx = 0;

	if (state == NULL || action == NULL || out_score == NULL || !ai_eval_is_valid_player(actor)) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	if (ai_state_discretize(state, actor, &discrete) != GAME_OK) {
		return GAME_ERROR_INVALID_STATE;
	}

	opponent = ai_eval_opponent(actor);
	actor_fighter = &state->combat.fighters[actor];
	opponent_fighter = &state->combat.fighters[opponent];
	abs_dx = ai_eval_abs(discrete.opponent_distance_x);

	if (!actor_fighter->alive) {
		*out_score = -1000;
		return GAME_OK;
	}

	if (discrete.local_zone == AI_LOCAL_ZONE_HAZARD) {
		score -= 55;
	}
	if (discrete.obstacle_between == AI_LOCAL_OBSTACLE_SOLID) {
		score -= 10;
	}
	if (discrete.obstacle_between == AI_LOCAL_OBSTACLE_HAZARD) {
		score -= 20;
	}

	switch (discrete.danger) {
		case AI_LOCAL_DANGER_HIGH:
			score -= 35;
			break;
		case AI_LOCAL_DANGER_LOW:
			score -= 10;
			break;
		case AI_LOCAL_DANGER_NONE:
		default:
			score += 10;
			break;
	}

	switch (action->type) {
		case ACTION_ATTACK:
			score += 70;
			if (abs_dx <= 1 && discrete.obstacle_between == AI_LOCAL_OBSTACLE_NONE) {
				score += 35;
			}
			if (actor_fighter->sword_height == opponent_fighter->sword_height) {
				score += 25;
			} else {
				score -= 35;
			}
			if (discrete.local_zone == AI_LOCAL_ZONE_HAZARD) {
				score -= 70;
			}
			if (discrete.obstacle_between != AI_LOCAL_OBSTACLE_NONE) {
				score -= 90;
			}
			break;
		case ACTION_PARRY:
			score += discrete.danger == AI_LOCAL_DANGER_HIGH ? 95 : 35;
			if (abs_dx > 2) {
				score -= 30;
			}
			if (actor_fighter->sword_height == opponent_fighter->sword_height) {
				score += 20;
			}
			break;
		case ACTION_HEIGHT_CHANGE:
			score += 25;
			if (action->sword_height == opponent_fighter->sword_height) {
				score += 30;
			}
			if (abs_dx <= 1) {
				score += 15;
			}
			if (action->sword_height == actor_fighter->sword_height) {
				score -= 20;
			}
			break;
		case ACTION_MOVE_LEFT:
		case ACTION_MOVE_RIGHT: {
			const bool toward_opponent = (discrete.opponent_distance_x > 0 && action->type == ACTION_MOVE_RIGHT) ||
				(discrete.opponent_distance_x < 0 && action->type == ACTION_MOVE_LEFT);
			if (toward_opponent) {
				score += abs_dx > 1 ? 45 : -30;
				if (discrete.obstacle_between == AI_LOCAL_OBSTACLE_NONE) {
					score += 10;
				}
				if (discrete.obstacle_between == AI_LOCAL_OBSTACLE_HAZARD) {
					score -= 60;
				}
				if (discrete.obstacle_between == AI_LOCAL_OBSTACLE_SOLID) {
					score -= 40;
				}
			} else {
				score += 15;
				if (discrete.local_zone == AI_LOCAL_ZONE_HAZARD) {
					score += 45;
				}
				if (discrete.danger == AI_LOCAL_DANGER_HIGH) {
					score += 30;
				}
				if (abs_dx > 3) {
					score -= 20;
				}
			}
			break;
		}
		case ACTION_NONE:
			score -= 25;
			break;
		case ACTION_JUMP:
		default:
			score = -500;
			break;
	}

	*out_score = score;
	return GAME_OK;
}

GameError ai_evaluator_score_state(const GameState *state, PlayerId perspective, int32_t *out_score) {
	AiDiscreteState discrete = {0};
	const FighterState *actor_fighter = NULL;
	const FighterState *opponent_fighter = NULL;
	PlayerId opponent = PLAYER_ONE;
	int32_t score = 0;
	int32_t abs_dx = 0;
	GameError err = GAME_OK;

	if (state == NULL || out_score == NULL || !ai_eval_is_valid_player(perspective)) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	err = ai_state_discretize(state, perspective, &discrete);
	if (err != GAME_OK) {
		return err;
	}

	opponent = ai_eval_opponent(perspective);
	actor_fighter = &state->combat.fighters[perspective];
	opponent_fighter = &state->combat.fighters[opponent];
	abs_dx = ai_eval_abs(discrete.opponent_distance_x);

	if (actor_fighter->alive) {
		score += 200;
	} else {
		score -= 1000;
	}

	if (opponent_fighter->alive) {
		score -= 120;
	} else {
		score += 1000;
	}

	if (discrete.local_zone == AI_LOCAL_ZONE_HAZARD) {
		score -= 80;
	}
	if (discrete.obstacle_between == AI_LOCAL_OBSTACLE_SOLID) {
		score -= 10;
	}
	if (discrete.obstacle_between == AI_LOCAL_OBSTACLE_HAZARD) {
		score -= 25;
	}

	switch (discrete.danger) {
		case AI_LOCAL_DANGER_HIGH:
			score -= 55;
			break;
		case AI_LOCAL_DANGER_LOW:
			score -= 20;
			break;
		case AI_LOCAL_DANGER_NONE:
		default:
			score += 15;
			break;
	}

	if (abs_dx <= 1 && actor_fighter->sword_height == opponent_fighter->sword_height) {
		score += 35;
	} else if (abs_dx > 3) {
		score -= abs_dx * 5;
	}

	score += (int32_t)actor_fighter->successful_parries * 5;
	score -= (int32_t)opponent_fighter->successful_parries * 5;
	score += (int32_t)opponent_fighter->stun_frames * 20;
	score -= (int32_t)actor_fighter->stun_frames * 20;

	*out_score = score;
	return GAME_OK;
}

GameError ai_evaluator_generate_candidates(const GameState *state, PlayerId actor, AiActionList *out_actions) {
	AiDiscreteState discrete = {0};
	const FighterState *actor_fighter = NULL;
	const FighterState *opponent_fighter = NULL;
	PlayerId opponent = PLAYER_ONE;
	Action action = {0};
	Action toward = {0};
	Action away = {0};
	int32_t score = 0;
	GameError err = GAME_OK;

	if (state == NULL || out_actions == NULL || !ai_eval_is_valid_player(actor)) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	out_actions->count = 0U;
	err = ai_state_discretize(state, actor, &discrete);
	if (err != GAME_OK) {
		return err;
	}

	opponent = ai_eval_opponent(actor);
	actor_fighter = &state->combat.fighters[actor];
	opponent_fighter = &state->combat.fighters[opponent];

	if (!actor_fighter->alive) {
		action = ai_eval_default_action();
		ai_eval_insert_candidate(out_actions, &action, 0);
		return GAME_OK;
	}

	toward = ai_eval_default_action();
	away = ai_eval_default_action();
	toward.type = discrete.opponent_distance_x >= 0 ? ACTION_MOVE_RIGHT : ACTION_MOVE_LEFT;
	away.type = toward.type == ACTION_MOVE_RIGHT ? ACTION_MOVE_LEFT : ACTION_MOVE_RIGHT;
	toward.sword_height = actor_fighter->sword_height;
	away.sword_height = actor_fighter->sword_height;

	if (ai_eval_abs(discrete.opponent_distance_x) <= 1 && discrete.obstacle_between == AI_LOCAL_OBSTACLE_NONE) {
		action = ai_eval_default_action();
		action.type = ACTION_ATTACK;
		action.sword_height = actor_fighter->sword_height;
		if (ai_evaluator_score_action(state, actor, &action, &score) == GAME_OK) {
			ai_eval_insert_candidate(out_actions, &action, score);
		}

		action = ai_eval_default_action();
		action.type = ACTION_PARRY;
		action.sword_height = actor_fighter->sword_height;
		if (ai_evaluator_score_action(state, actor, &action, &score) == GAME_OK) {
			ai_eval_insert_candidate(out_actions, &action, score);
		}
	}

	if (actor_fighter->sword_height != opponent_fighter->sword_height || ai_eval_abs(discrete.opponent_distance_x) <= 2) {
		action = ai_eval_default_action();
		action.type = ACTION_HEIGHT_CHANGE;
		action.sword_height = ai_eval_preferred_height(actor_fighter, opponent_fighter);
		if (ai_evaluator_score_action(state, actor, &action, &score) == GAME_OK) {
			ai_eval_insert_candidate(out_actions, &action, score);
		}
	}

	if (discrete.obstacle_between != AI_LOCAL_OBSTACLE_HAZARD &&
			discrete.obstacle_between != AI_LOCAL_OBSTACLE_SOLID) {
		if (ai_evaluator_score_action(state, actor, &toward, &score) == GAME_OK) {
			ai_eval_insert_candidate(out_actions, &toward, score);
		}
	}

	if (discrete.local_zone == AI_LOCAL_ZONE_HAZARD ||
			discrete.danger != AI_LOCAL_DANGER_NONE ||
			discrete.obstacle_between != AI_LOCAL_OBSTACLE_NONE) {
		if (ai_evaluator_score_action(state, actor, &away, &score) == GAME_OK) {
			ai_eval_insert_candidate(out_actions, &away, score);
		}
	}

	if (out_actions->count == 0U) {
		action = ai_eval_default_action();
		if (ai_evaluator_score_action(state, actor, &action, &score) == GAME_OK) {
			ai_eval_insert_candidate(out_actions, &action, score);
		}
	}

	return GAME_OK;
}

GameError ai_evaluator_apply_action(const GameState *state, PlayerId actor, const Action *action, GameState *out_state) {
	PlayerId opponent = PLAYER_ONE;
	FighterState *actor_fighter = NULL;
	FighterState *opponent_fighter = NULL;
	int32_t actor_x = 0;
	int32_t actor_y = 0;
	int32_t target_x = 0;
	int32_t opponent_x = 0;
	int32_t opponent_y = 0;
	GameError err = GAME_OK;

	if (state == NULL || action == NULL || out_state == NULL || !ai_eval_is_valid_player(actor)) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	*out_state = *state;
	opponent = ai_eval_opponent(actor);
	actor_fighter    = &out_state->combat.fighters[actor];
	opponent_fighter = &out_state->combat.fighters[opponent];
	actor_x    = ai_eval_px_to_tile_x(actor_fighter->position.x);
	actor_y    = ai_eval_px_to_tile_y(actor_fighter->position.y);
	opponent_x = ai_eval_px_to_tile_x(opponent_fighter->position.x);
	opponent_y = ai_eval_px_to_tile_y(opponent_fighter->position.y);

	switch (action->type) {
		case ACTION_MOVE_LEFT:
		case ACTION_MOVE_RIGHT:
			target_x = actor_x + (action->type == ACTION_MOVE_RIGHT ? 1 : -1);
			if (ai_eval_inside_arena(&out_state->arena, target_x, actor_y) &&
					target_x != opponent_x) {
				const TileType tile = ai_eval_tile_at(&out_state->arena, target_x, actor_y);
				if (tile != TILE_SOLID && tile != TILE_PLATFORM) {
					actor_fighter->position.x = (float)(target_x * ARENA_TILE_W);
					if (tile == TILE_HAZARD) {
						actor_fighter->alive = false;
					}
				}
			}
			break;
		case ACTION_ATTACK:
			err = ai_state_discretize(out_state, actor, &(AiDiscreteState){0});
			if (err == GAME_OK && actor_fighter->alive && opponent_fighter->alive &&
					ai_eval_abs(opponent_x - actor_x) <= 1 && ai_eval_abs(opponent_y - actor_y) <= 1 &&
					actor_fighter->sword_height == opponent_fighter->sword_height) {
				opponent_fighter->alive = false;
			}
			break;
		case ACTION_PARRY:
			actor_fighter->successful_parries++;
			if (ai_eval_abs(opponent_x - actor_x) <= 1 && actor_fighter->sword_height == opponent_fighter->sword_height) {
				opponent_fighter->stun_frames = 2U;
			}
			break;
		case ACTION_HEIGHT_CHANGE:
			actor_fighter->sword_height = action->sword_height;
			break;
		case ACTION_NONE:
			break;
		case ACTION_JUMP:
		default:
			return GAME_ERROR_UNSUPPORTED;
	}

	if (actor_fighter->stun_frames > 0U) {
		actor_fighter->stun_frames--;
	}
	if (opponent_fighter->stun_frames > 0U) {
		opponent_fighter->stun_frames--;
	}
	out_state->frame_index++;
	return GAME_OK;
}

GameError ai_evaluator_pick_fallback(const GameState *state, PlayerId actor, Action *out_action) {
	AiActionList actions = {0};
	uint8_t idx = 0U;
	GameError err = GAME_OK;

	if (out_action == NULL) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	*out_action = ai_eval_default_action();
	if (state == NULL || !ai_eval_is_valid_player(actor)) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	err = ai_evaluator_generate_candidates(state, actor, &actions);
	if (err != GAME_OK) {
		return err;
	}

	for (idx = 0U; idx < actions.count; ++idx) {
		if (actions.items[idx].action.type == ACTION_PARRY) {
			*out_action = actions.items[idx].action;
			return GAME_OK;
		}
	}

	for (idx = 0U; idx < actions.count; ++idx) {
		if (actions.items[idx].action.type == ACTION_MOVE_LEFT || actions.items[idx].action.type == ACTION_MOVE_RIGHT) {
			*out_action = actions.items[idx].action;
			return GAME_OK;
		}
	}

	if (actions.count > 0U) {
		*out_action = actions.items[0].action;
	}

	return GAME_OK;
}
