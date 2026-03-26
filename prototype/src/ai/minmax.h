#ifndef AI_MINMAX_H
#define AI_MINMAX_H

#include "types.h"
#include "ai/state.h"

/**
 * MinMax with Alpha-Beta Pruning
 * 
 * From architecture: IA <1ms/frame budget
 * Search tree prunes branches that cannot affect the outcome
 * 
 * Time budget: AI_TIME_BUDGET_US microseconds per call
 * Depth: Determined by difficulty level
 */

typedef struct {
    Action best_action;
    int32_t score;
    uint32_t nodes_evaluated;
} MinMaxResult;

/**
 * Run MinMax with time budget
 * Returns best action found within time limit
 */
MinMaxResult minmax_search(const CombatState *state, const Arena *arena,
                           PlayerId actor, uint8_t max_depth,
                           uint32_t time_budget_us);

#endif