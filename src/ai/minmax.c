#include "ai/minmax.h"
#include "ai/state.h"
#include "ai/evaluator.h"
#include "combat.h"
#include "constants.h"
#include <SDL3/SDL.h>
#include <string.h>

#define SCORE_WIN  10000
#define SCORE_LOSE -10000
#define SCORE_NEG_INF -32768
#define SCORE_POS_INF  32767

/**
 * All possible AI actions (branching set)
 * Architecture: MinMax evalutes discrete action set
 */
static const ActionType AI_ACTION_TYPES[] = {
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_ATTACK,
    ACTION_PARRY,
    ACTION_NONE
};

static const SwordHeight SWORD_HEIGHTS[] = {
    SWORD_HEIGHT_LOW,
    SWORD_HEIGHT_MID,
    SWORD_HEIGHT_HIGH
};

// Time budget tracking (shared across recursive calls)
static uint64_t s_search_start_us = 0;
static uint32_t s_budget_us = 0;
static uint32_t s_nodes = 0;

static bool time_exceeded(void) {
    uint64_t elapsed = SDL_GetTicks() * 1000 - s_search_start_us;
    return elapsed >= s_budget_us;
}

/**
 * Apply an action to a copy of the combat state
 */
static CombatState apply_action_to_state(const CombatState *base,
                                          const Arena *arena,
                                          PlayerId actor,
                                          ActionType action_type,
                                          SwordHeight height) {
    CombatState sim = *base;  // Shallow copy
    
    // Apply action to simulation
    Action action = {
        .type = action_type,
        .sword_height = height,
        .issued_frame = base->round_time_frames
    };
    
    // Simulate the action
    if (action.type == ACTION_MOVE_LEFT) {
        sim.fighters[actor].velocity.x = -5.0f;
    } else if (action.type == ACTION_MOVE_RIGHT) {
        sim.fighters[actor].velocity.x = 5.0f;
    } else if (action.type == ACTION_ATTACK) {
        sim.fighters[actor].sword_height = height;
    } else if (action.type == ACTION_PARRY) {
        // Parry: assume attempt to block
        sim.fighters[actor].stun_frames = 0;
    } else {
        sim.fighters[actor].velocity.x = 0;
    }
    
    // Update sword height
    sim.fighters[actor].sword_height = height;
    
    // Advance simulation one step
    combat_step(&sim, arena, FIXED_TIMESTEP_MS);
    
    return sim;
}

/**
 * MinMax alpha-beta recursive search
 */
static int32_t alphabeta(const CombatState *state,
                          const Arena *arena,
                          PlayerId maximizing_player,
                          uint8_t depth,
                          int32_t alpha, int32_t beta,
                          bool is_maximizing) {
    s_nodes++;
    
    // Terminal conditions
    if (depth == 0 || time_exceeded()) {
        // Convert CombatState to AIState for evaluator
        GameState temp_gs = {0};
        temp_gs.combat = *state;
        temp_gs.arena = *arena;
        AIState ai_state = ai_state_discretize(&temp_gs);
        return evaluator_score(&ai_state, maximizing_player, arena);
    }
    
    PlayerId current_actor = is_maximizing ? maximizing_player 
                                           : (maximizing_player == PLAYER_ONE ? PLAYER_TWO : PLAYER_ONE);
    
    int num_action_types = 5;  // sizeof(AI_ACTION_TYPES)
    int num_heights = 3;
    
    if (is_maximizing) {
        int32_t max_score = SCORE_NEG_INF;
        
        for (int a = 0; a < num_action_types; a++) {
            for (int h = 0; h < num_heights; h++) {
                if (time_exceeded()) goto done_max;
                
                CombatState next = apply_action_to_state(
                    state, arena, current_actor,
                    AI_ACTION_TYPES[a], SWORD_HEIGHTS[h]
                );
                
                int32_t score = alphabeta(&next, arena, maximizing_player,
                                          depth - 1, alpha, beta, false);
                
                if (score > max_score) max_score = score;
                if (score > alpha) alpha = score;
                if (alpha >= beta) goto done_max;  // Beta cutoff
            }
        }
        done_max:
        return max_score;
    } else {
        int32_t min_score = SCORE_POS_INF;
        
        for (int a = 0; a < num_action_types; a++) {
            for (int h = 0; h < num_heights; h++) {
                if (time_exceeded()) goto done_min;
                
                CombatState next = apply_action_to_state(
                    state, arena, current_actor,
                    AI_ACTION_TYPES[a], SWORD_HEIGHTS[h]
                );
                
                int32_t score = alphabeta(&next, arena, maximizing_player,
                                          depth - 1, alpha, beta, true);
                
                if (score < min_score) min_score = score;
                if (score < beta) beta = score;
                if (alpha >= beta) goto done_min;  // Alpha cutoff
            }
        }
        done_min:
        return min_score;
    }
}

/**
 * Main MinMax search entry point
 * 
 * Time-budgeted: will stop searching when time_budget_us is exceeded
 * Iterative deepening: starts at depth 1, increases until budget exhausted
 */
MinMaxResult minmax_search(const CombatState *state, const Arena *arena,
                           PlayerId actor, uint8_t max_depth,
                           uint32_t time_budget_us) {
    MinMaxResult result = {0};
    result.score = SCORE_NEG_INF;
    
    // Set initial safe action as fallback
    result.best_action = (Action){
        .type = ACTION_NONE,
        .sword_height = SWORD_HEIGHT_MID,
        .issued_frame = state->round_time_frames
    };
    
    if (state == NULL || arena == NULL) return result;
    
    // Initialize time budget
    s_search_start_us = SDL_GetTicks() * 1000;
    s_budget_us = time_budget_us;
    s_nodes = 0;
    
    int num_action_types = 5;
    int num_heights = 3;
    
    // Try each possible first action
    for (int a = 0; a < num_action_types; a++) {
        for (int h = 0; h < num_heights; h++) {
            if (time_exceeded()) goto done;
            
            CombatState next = apply_action_to_state(
                state, arena, actor,
                AI_ACTION_TYPES[a], SWORD_HEIGHTS[h]
            );
            
            int32_t score = alphabeta(&next, arena, actor,
                                      max_depth - 1,
                                      SCORE_NEG_INF, SCORE_POS_INF,
                                      false);  // Opponent minimizes
            
            if (score > result.score) {
                result.score = score;
                result.best_action.type = AI_ACTION_TYPES[a];
                result.best_action.sword_height = SWORD_HEIGHTS[h];
                result.best_action.issued_frame = state->round_time_frames;
            }
        }
    }
    done:
    
    result.nodes_evaluated = s_nodes;
    return result;
}
