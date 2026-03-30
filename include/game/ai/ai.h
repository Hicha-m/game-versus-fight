#ifndef AI_H
#define AI_H

#include "ai_types.h"
#include "game/arena/arena.h"
#include "game/combat/combat.h"

typedef struct AIHeuristicWeights {
    f32 distance_weight;
    f32 sword_advantage_weight;
    f32 height_advantage_weight;
    f32 center_control_weight;
    f32 edge_danger_weight;
    f32 attack_opportunity_weight;
    f32 stun_penalty_weight;
} AIHeuristicWeights;

typedef struct AIMetrics {
    u64 nodes_expanded;
    u64 leaves_evaluated;
    u64 cutoffs;
    i32 max_depth_reached;
    f64 think_time_ms;
    u64 estimated_memory_bytes;
} AIMetrics;

/* État discret destiné à l'algorithme */
typedef struct AIDecisionState {
    i32 current_room;
    i32 room_count;

    i32 self_x_bucket;
    i32 self_y_bucket;
    i32 enemy_x_bucket;
    i32 enemy_y_bucket;

    i32 room_max_x_bucket;
    i32 objective_direction;

    bool self_grounded;
    bool enemy_grounded;

    bool self_has_sword;
    bool enemy_has_sword;

    SwordLine self_sword_line;
    SwordLine enemy_sword_line;

    bool self_near_edge;
    bool enemy_near_edge;

    bool self_can_thrust;
    bool enemy_can_thrust;
} AIDecisionState;

typedef struct AIController {
    bool enabled;

    AIDifficulty difficulty;
    AIAlgorithm algorithm;
    AIGameplayMode gameplay_mode;

    i32 search_depth;
    f32 randomness;

    AIHeuristicWeights weights;
    AIMetrics metrics;
} AIController;

/* Lifecycle */
bool ai_init(AIController* ai);
void ai_shutdown(AIController* ai);

void ai_default_weights(AIHeuristicWeights* out_weights);


/* Configuration */
void ai_set_difficulty(AIController* ai, AIDifficulty difficulty);
void ai_set_algorithm(AIController* ai, AIAlgorithm algorithm);
void ai_set_gameplay_mode(AIController* ai, AIGameplayMode mode);
void ai_reset_metrics(AIController* ai);

/* Heuristiques selon le mode de gameplay */
void ai_weights_for_mode(AIHeuristicWeights* out_weights, AIGameplayMode mode);

/* État discret */
void ai_build_decision_state(
    AIDecisionState* out_state,
    const Arena* arena,
    const CombatState* combat,
    i32 self_index
);

/* Évaluation heuristique */
f32 ai_evaluate_state(const AIDecisionState* state, const AIHeuristicWeights* weights);

/* Décision */
PlayerCommand ai_think(
    AIController* ai,
    const Arena* arena,
    const CombatState* combat,
    i32 self_index,
    f32 dt,
    const i32* player_kills  /* Array of 2 ints: kills[0] and kills[1] */
);

#endif

