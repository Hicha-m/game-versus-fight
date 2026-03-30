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
    AI_MODE_DEFENSIVE = 0,   /* EASY: évite le combat, sécurité d'abord */
    AI_MODE_BALANCED,        /* MEDIUM: mix équilibré attaque/défense */
    AI_MODE_TACTICAL,        /* HARD: attaque intelligente + strategy */
    AI_MODE_AGGRESSIVE       /* EXPERT: maximize les dégâts, tue l'ennemi */
} AIGameplayMode;

#endif
