#ifndef AI_INTERNAL_H
#define AI_INTERNAL_H

#include "game/ai/ai.h"

typedef enum AIAction {
    AI_ACTION_ADVANCE = 0,
    AI_ACTION_RETREAT,
    AI_ACTION_JUMP,
    AI_ACTION_THRUST,
    AI_ACTION_LINE_HIGH,
    AI_ACTION_LINE_MID,
    AI_ACTION_LINE_LOW,
    AI_ACTION_THROW
} AIAction;

PlayerCommand ai_action_to_command(AIAction action);

#endif