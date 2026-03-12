#include "ai/state.h"
#include "types.h"

/**
 * Convert floating-point GameState into compact discrete AIState
 * for use in MinMax search tree
 */
AIState ai_state_discretize(const GameState *state) {
    AIState ai_state = {0};
    
    if (state == NULL) return ai_state;
    
    const CombatState *combat = &state->combat;
    
    for (int i = 0; i < 2; i++) {
        const FighterState *f = &combat->fighters[i];
        
        // Discretize position to grid units
        ai_state.fighter_x[i] = (int16_t)f->position.x;
        ai_state.fighter_y[i] = (int8_t)f->position.y;
        
        // Sword height, grounded status
        ai_state.sword_height[i] = f->sword_height;
        ai_state.grounded[i] = f->grounded;
        
        // Discretize momentum: 0 = none, 1 = low (<30 frames), 2 = high (>=30)
        if (f->momentum_frames == 0) ai_state.momentum_level[i] = 0;
        else if (f->momentum_frames < 30) ai_state.momentum_level[i] = 1;
        else ai_state.momentum_level[i] = 2;
        
        // Parry count modulo threshold (for sword evolution)
        ai_state.parry_count[i] = (uint8_t)(f->successful_parries % 3);
        ai_state.invulnerable[i] = (f->invulnerability_frames > 0) ? 1 : 0;
    }
    
    return ai_state;
}

/**
 * Simple hash of AIState for transposition table
 * FNV-1a inspired hash
 */
uint32_t ai_state_hash(const AIState *state) {
    if (state == NULL) return 0;
    
    const uint8_t *bytes = (const uint8_t *)state;
    uint32_t hash = 2166136261u;  // FNV offset basis
    
    for (size_t i = 0; i < sizeof(AIState); i++) {
        hash ^= bytes[i];
        hash *= 16777619u;  // FNV prime
    }
    
    return hash;
}
