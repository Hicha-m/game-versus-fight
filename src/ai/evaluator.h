#ifndef AI_EVALUATOR_H
#define AI_EVALUATOR_H

#include "types.h"
#include "ai/state.h"

/**
 * Heuristic evaluation function
 * 
 * Evaluates how beneficial a game state is for the given perspective
 * 
 * Evaluation factors (from GDD/Architecture):
 * 1. Positional: Distance from goal (higher weight - tug-of-war)
 * 2. Momentum: Current momentum level (speed/attack bonus)
 * 3. Sword evolution: Parry count and reach bonus
 * 4. Sword height advantage (based on opponent's height)
 * 5. Alive status
 * 
 * Returns: Score [-10000, 10000]
 *   +10000 = AI wins
 *   -10000 = AI loses
 *   0 = neutral
 */
int32_t evaluator_score(const AIState *state, PlayerId perspective, const Arena *arena);

#endif