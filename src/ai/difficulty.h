#ifndef AI_DIFFICULTY_H
#define AI_DIFFICULTY_H

#include "types.h"

typedef struct AiDifficultyConfig {
	uint8_t search_depth;
	uint8_t randomness_percent;
	uint32_t decision_budget_us;
} AiDifficultyConfig;

DifficultyLevel ai_difficulty_sanitize(DifficultyLevel difficulty);
GameError ai_difficulty_get_config(DifficultyLevel difficulty, AiDifficultyConfig *out_config);

#endif