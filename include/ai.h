#ifndef AI_H
#define AI_H

#include <stdint.h>

#include "types.h"

Action ai_get_action(const GameState *state, DifficultyLevel difficulty);

// MinMax with Alpha-Beta Pruning
GameError ai_choose_action(const GameState *state, PlayerId actor, uint32_t budget_us, Action *out_action);
//heuristic of scoring
int32_t ai_evaluate_state(const GameState *state, PlayerId perspective);
//config
GameError ai_set_difficulty(GameState *state, DifficultyLevel difficulty);
//MinMax depth
uint8_t ai_search_depth_for_difficulty(DifficultyLevel difficulty);

#endif
