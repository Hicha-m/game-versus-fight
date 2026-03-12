#ifndef AI_STATE_H
#define AI_STATE_H

#include "types.h"

/**
 * Discrete AI State
 * 
 * The AI evaluates a compact, discretized state of the world
 * instead of raw floating-point positions.
 * 
 * Reasons for discretization:
 * - Reduces branching factor in MinMax tree
 * - Enables memoization (transposition table)
 * - Makes evaluation deterministic
 */
typedef struct {
    int16_t fighter_x[2];       // Grid X positions
    int8_t  fighter_y[2];       // Grid Y positions (height level)
    SwordHeight sword_height[2]; // Each fighter's sword height
    bool    grounded[2];         // Is each fighter on ground
    uint8_t momentum_level[2];   // 0 = none, 1 = low, 2 = high
    uint8_t parry_count[2];      // Cumulative parry count (mod 3)
    uint8_t invulnerable[2];     // Is fighter temporarily invulnerable
} AIState;

/**
 * Convert continuous GameState to discrete AIState
 */
AIState ai_state_discretize(const GameState *state);

/**
 * Hash function for transposition table
 */
uint32_t ai_state_hash(const AIState *state);

#endif