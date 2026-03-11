#ifndef AI_MINMAX_H
#define AI_MINMAX_H

#include <stdbool.h>

#include "evaluator.h"

typedef struct AiSearchResult {
	Action action;
	int32_t score;
	uint32_t explored_nodes;
	uint32_t pruned_nodes;
	uint8_t completed_depth;
	bool timed_out;
} AiSearchResult;

GameError ai_minmax_search(const GameState *state, PlayerId actor, PlayerId perspective,
		uint8_t depth, uint32_t budget_us, AiSearchResult *out_result);
void ai_minmax_set_test_node_budget(uint32_t max_nodes);
void ai_minmax_reset_test_node_budget(void);

#endif