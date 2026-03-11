#include "minmax.h"

#include <limits.h>
#include <time.h>

typedef struct AiMinmaxContext {
	PlayerId perspective;
	uint64_t deadline_us;
	uint32_t explored_nodes;
	uint32_t pruned_nodes;
	bool timed_out;
} AiMinmaxContext;

static uint32_t g_ai_minmax_test_node_budget = 0U;

static uint64_t ai_minmax_now_us(void) {
	struct timespec now = {0};
	(void)timespec_get(&now, TIME_UTC);
	return ((uint64_t)now.tv_sec * 1000000ULL) + ((uint64_t)now.tv_nsec / 1000ULL);
}

static bool ai_minmax_is_valid_player(PlayerId player) {
	return player == PLAYER_ONE || player == PLAYER_TWO;
}

static PlayerId ai_minmax_next_player(PlayerId player) {
	return player == PLAYER_ONE ? PLAYER_TWO : PLAYER_ONE;
}

static Action ai_minmax_default_action(void) {
	Action action = {0};
	action.type = ACTION_NONE;
	action.sword_height = SWORD_HEIGHT_MID;
	action.issued_frame = 0U;
	return action;
}

static bool ai_minmax_budget_hit(AiMinmaxContext *ctx) {
	if (ctx->timed_out) {
		return true;
	}
	if (g_ai_minmax_test_node_budget > 0U && ctx->explored_nodes >= g_ai_minmax_test_node_budget) {
		ctx->timed_out = true;
		return true;
	}
	if (ctx->deadline_us > 0U && ai_minmax_now_us() >= ctx->deadline_us) {
		ctx->timed_out = true;
		return true;
	}
	return false;
}

static int32_t ai_minmax_leaf_score(const GameState *state, PlayerId perspective, AiMinmaxContext *ctx) {
	int32_t score = 0;
	if (ai_evaluator_score_state(state, perspective, &score) != GAME_OK) {
		ctx->timed_out = true;
		return 0;
	}
	return score;
}

static int32_t ai_minmax_visit(const GameState *state, PlayerId actor, uint8_t depth,
		int32_t alpha, int32_t beta, AiMinmaxContext *ctx, Action *out_action) {
	AiActionList actions = {0};
	int32_t best_score = 0;
	uint8_t idx = 0U;
	const bool maximizing = actor == ctx->perspective;
	Action best_action = ai_minmax_default_action();

	if (ai_minmax_budget_hit(ctx)) {
		return ai_minmax_leaf_score(state, ctx->perspective, ctx);
	}

	ctx->explored_nodes++;
	if (depth == 0U || !state->combat.fighters[PLAYER_ONE].alive || !state->combat.fighters[PLAYER_TWO].alive) {
		return ai_minmax_leaf_score(state, ctx->perspective, ctx);
	}

	if (ai_evaluator_generate_candidates(state, actor, &actions) != GAME_OK || actions.count == 0U) {
		return ai_minmax_leaf_score(state, ctx->perspective, ctx);
	}

	best_score = maximizing ? INT_MIN : INT_MAX;
	for (idx = 0U; idx < actions.count; ++idx) {
		GameState next_state = {0};
		Action candidate_action = actions.items[idx].action;
		int32_t child_score = 0;

		if (ai_evaluator_apply_action(state, actor, &candidate_action, &next_state) != GAME_OK) {
			continue;
		}

		child_score = ai_minmax_visit(&next_state, ai_minmax_next_player(actor), depth - 1U,
				alpha, beta, ctx, NULL);
		if (ctx->timed_out) {
			break;
		}

		if (maximizing) {
			if (child_score > best_score) {
				best_score = child_score;
				best_action = candidate_action;
			}
			if (child_score > alpha) {
				alpha = child_score;
			}
		} else {
			if (child_score < best_score) {
				best_score = child_score;
				best_action = candidate_action;
			}
			if (child_score < beta) {
				beta = child_score;
			}
		}

		if (beta <= alpha) {
			ctx->pruned_nodes += (uint32_t)(actions.count - idx - 1U);
			break;
		}
	}

	if (out_action != NULL) {
		*out_action = best_action;
	}
	if (best_score == INT_MIN || best_score == INT_MAX) {
		return ai_minmax_leaf_score(state, ctx->perspective, ctx);
	}
	return best_score;
}

GameError ai_minmax_search(const GameState *state, PlayerId actor, PlayerId perspective,
		uint8_t depth, uint32_t budget_us, AiSearchResult *out_result) {
	AiMinmaxContext ctx = {0};
	Action best_action = ai_minmax_default_action();

	if (state == NULL || out_result == NULL || !ai_minmax_is_valid_player(actor) || !ai_minmax_is_valid_player(perspective)) {
		return GAME_ERROR_INVALID_ARGUMENT;
	}

	out_result->action = best_action;
	out_result->score = 0;
	out_result->explored_nodes = 0U;
	out_result->pruned_nodes = 0U;
	out_result->completed_depth = 0U;
	out_result->timed_out = false;

	if (budget_us == 0U) {
		out_result->timed_out = true;
		return GAME_ERROR_TIMEOUT;
	}

	ctx.perspective = perspective;
	ctx.deadline_us = ai_minmax_now_us() + (uint64_t)budget_us;
	out_result->score = ai_minmax_visit(state, actor, depth, INT_MIN / 2, INT_MAX / 2, &ctx, &best_action);
	out_result->action = best_action;
	out_result->explored_nodes = ctx.explored_nodes;
	out_result->pruned_nodes = ctx.pruned_nodes;
	out_result->completed_depth = depth;
	out_result->timed_out = ctx.timed_out;

	return ctx.timed_out ? GAME_ERROR_TIMEOUT : GAME_OK;
}

void ai_minmax_set_test_node_budget(uint32_t max_nodes) {
	g_ai_minmax_test_node_budget = max_nodes;
}

void ai_minmax_reset_test_node_budget(void) {
	g_ai_minmax_test_node_budget = 0U;
}
