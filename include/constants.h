#ifndef CONSTANTS_H
#define CONSTANTS_H

// FPS and timing
#define TARGET_FPS 60U
#define FIXED_TIMESTEP_MS 16U
#define FIXED_TIMESTEP_SEC (FIXED_TIMESTEP_MS / 1000.0f)

// Input buffering
#define MAX_INPUT_BUFFER_FRAMES 3U

// AI budgets
#define AI_TIME_BUDGET_US 1000U

// Arena generation
#define MIN_ARENA_WIDTH 16U
#define MAX_ARENA_WIDTH 256U
#define MIN_ARENA_HEIGHT 8U
#define MAX_ARENA_HEIGHT 128U

// Match timing
#define MAX_ROUND_TIME_SECONDS 180U

// Momentum system (GDD: passive momentum buff)
#define MOMENTUM_DECAY_FRAMES 60U
#define MOMENTUM_SPEED_BONUS_MAX 1.5f  // 50% faster with max momentum

// Sword evolution system (GDD: 3 parries = +10% reach)
#define SWORD_REACH_BONUS 1.1f
#define PARRIES_FOR_SWORD_EVOLUTION 3U

// Physics - Movement
#define MAX_HORIZONTAL_VEL 10.0f    // units/sec max speed
#define ACCELERATION 3.0f           // units/sec² for speeding up
#define DECELERATION 3.0f           // units/sec² for slowing down
#define JUMP_FORCE 8.0f             // Initial vertical velocity
#define GRAVITY 9.81f               // units/sec² downward

// Combat mechanics
#define INVULNERABILITY_FRAMES 20U  // Frames of temporary invulnerability post-hit
#define PARRY_STARTUP_FRAMES 4U     // Frames before parry is active
#define PARRY_ACTIVE_FRAMES 8U      // Duration parry blocks hits
#define ATTACK_STARTUP_FRAMES 6U    // Frames of attack startup
#define ATTACK_ACTIVE_FRAMES 10U    // Duration hit can connect
#define STUN_FRAMES_ON_PARRY 10U    // Stun attacker when parried

// Reach/hitbox sizes (tiles)
#define BASE_SWORD_REACH 2.0f       // Units of reach
#define PLAYER_WIDTH 1.0f
#define PLAYER_HEIGHT 2.0f

#endif
