#ifndef AI_STATE_H
#define AI_STATE_H

#include "types.h"

typedef enum AiLocalDanger {
	AI_LOCAL_DANGER_NONE,
	AI_LOCAL_DANGER_LOW,
	AI_LOCAL_DANGER_HIGH
} AiLocalDanger;

typedef enum AiLocalObstacle {
	AI_LOCAL_OBSTACLE_NONE,
	AI_LOCAL_OBSTACLE_SOLID,
	AI_LOCAL_OBSTACLE_HAZARD
} AiLocalObstacle;

/* Keep boost/slow now to avoid changing downstream APIs when these tiles are added. */
typedef enum AiLocalZone {
	AI_LOCAL_ZONE_NEUTRAL,
	AI_LOCAL_ZONE_HAZARD,
	AI_LOCAL_ZONE_BOOST,
	AI_LOCAL_ZONE_SLOW
} AiLocalZone;

typedef struct AiDiscreteState {
	AiLocalDanger danger;
	AiLocalObstacle obstacle_between;
	AiLocalZone local_zone;
	uint8_t actor_grounded;
	uint8_t actor_alive;
	SwordHeight actor_sword_height;
	int16_t opponent_distance_x;
	int16_t opponent_distance_y;
} AiDiscreteState;

GameError ai_state_discretize(const GameState *state, PlayerId actor, AiDiscreteState *out_state);

#endif