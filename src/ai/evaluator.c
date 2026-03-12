#include "ai/evaluator.h"
#include "constants.h"

/**
 * Heuristic evaluation of a discrete AIState from a given perspective
 * 
 * Factors:
 * 1. Position toward goal (biggest factor)
 * 2. Momentum speed advantage
 * 3. Sword reach advantage (parry count)
 * 4. Sword height matchup advantage
 * 5. Invulnerability frames (temporary safety)
 */
int32_t evaluator_score(const AIState *state, PlayerId perspective, const Arena *arena) {
    if (state == NULL) return 0;
    
    PlayerId opponent = (perspective == PLAYER_ONE) ? PLAYER_TWO : PLAYER_ONE;
    
    int32_t score = 0;
    
    // 1. Position relative to goals
    // Player 1 wants to move RIGHT (toward high X)
    // Player 2 wants to move LEFT (toward low X)
    int16_t arena_width = arena ? arena->width : 32;
    
    int32_t my_x = state->fighter_x[perspective];
    int32_t opp_x = state->fighter_x[opponent];
    
    if (perspective == PLAYER_ONE) {
        // P1: wants to be at high X, opponent at low X
        score += (my_x - opp_x) * 50;  // Higher weight for position than combat advantage
    } else {
        // P2: wants to be at low X, opponent at high X
        score += (opp_x - my_x) * 50;
    }
    
    // 2. Momentum advantage (being faster = better)
    int8_t mom_diff = (int8_t)state->momentum_level[perspective] - (int8_t)state->momentum_level[opponent];
    score += mom_diff * 30;
    
    // 3. Sword reach advantage (parry count near evolution threshold)
    // Having parries ready = near increase
    int8_t parry_diff = (int8_t)state->parry_count[perspective] - (int8_t)state->parry_count[opponent];
    score += parry_diff * 20;
    
    // 4. Sword height advantage (rock-paper-scissors)
    // Having the advantageous height over opponent's height
    SwordHeight my_height = state->sword_height[perspective];
    SwordHeight opp_height = state->sword_height[opponent];
    
    if (my_height == SWORD_HEIGHT_HIGH && opp_height == SWORD_HEIGHT_LOW) {
        score += 40;  // HIGH beats LOW
    } else if (my_height == SWORD_HEIGHT_LOW && opp_height == SWORD_HEIGHT_HIGH) {
        score += 40;  // LOW beats HIGH
    } else if (my_height == opp_height) {
        score += 0;   // Same height = neutral
    } else {
        score -= 20;  // No clear advantage
    }
    
    // 5. Invulnerability/defensive bonus
    if (state->invulnerable[opponent] && !state->invulnerable[perspective]) {
        score -= 30;  // Opponent is untouchable temporarily
    }
    if (state->invulnerable[perspective] && !state->invulnerable[opponent]) {
        score += 30;  // We are safe
    }
    
    // Boundary bonus: being close to their own goal
    int16_t goal_dist;
    if (perspective == PLAYER_ONE) {
        goal_dist = arena_width - 1 - my_x;  // Distance to right side
    } else {
        goal_dist = my_x;  // Distance to left side
    }
    score += (arena_width - goal_dist) * 5;  // Closer to goal = better
    
    return score;
}
