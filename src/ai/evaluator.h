#ifndef AI_EVALUATOR_H
#define AI_EVALUATOR_H

#include "state.h"

#define AI_MAX_CANDIDATE_ACTIONS 5U

typedef struct AiCandidateAction {
	Action action;
	int32_t score;
} AiCandidateAction;

typedef struct AiActionList {
	AiCandidateAction items[AI_MAX_CANDIDATE_ACTIONS];
	uint8_t count;
} AiActionList;

GameError ai_evaluator_generate_candidates(const GameState *state, PlayerId actor, AiActionList *out_actions);
GameError ai_evaluator_score_action(const GameState *state, PlayerId actor, const Action *action, int32_t *out_score);
GameError ai_evaluator_score_state(const GameState *state, PlayerId perspective, int32_t *out_score);
GameError ai_evaluator_apply_action(const GameState *state, PlayerId actor, const Action *action, GameState *out_state);
GameError ai_evaluator_pick_fallback(const GameState *state, PlayerId actor, Action *out_action);

#endif