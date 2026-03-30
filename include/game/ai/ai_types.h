#ifndef AI_TYPES_H
#define AI_TYPES_H

#include "core/types.h"

typedef enum AIDifficulty {
    AI_DIFFICULTY_EASY = 0,
    AI_DIFFICULTY_MEDIUM,
    AI_DIFFICULTY_HARD,
    AI_DIFFICULTY_EXPERT
} AIDifficulty;

typedef enum AIAlgorithm {
    AI_ALGO_SCRIPTED = 0,
    AI_ALGO_MINIMAX,
    AI_ALGO_MINIMAX_ALPHA_BETA
} AIAlgorithm;

typedef enum AIGameplayMode {
    AI_MODE_DEFENSIVE = 0,
    AI_MODE_BALANCED,
    AI_MODE_TACTICAL,
    AI_MODE_AGGRESSIVE
} AIGameplayMode;

#endif
