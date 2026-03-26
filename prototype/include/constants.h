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
#define MAX_ARENA_WIDTH 320U
#define MIN_ARENA_HEIGHT 8U
#define MAX_ARENA_HEIGHT 128U

// Match timing
#define MIN_ROUND_TIME_SECONDS 60U
#define DEFAULT_ROUND_TIME_SECONDS 300U
#define MAX_ROUND_TIME_SECONDS 600U
#define ROUND_TIME_STEP_SECONDS 30U

// Momentum system (GDD: passive momentum buff)
#define MOMENTUM_DECAY_FRAMES 60U
#define MOMENTUM_SPEED_BONUS_MAX 1.5f  // 50% faster with max momentum

// Sword evolution system (GDD: 3 parries = +10% reach)
#define SWORD_REACH_BONUS 1.1f
#define PARRIES_FOR_SWORD_EVOLUTION 3U

// Physics - Movement
#define MAX_HORIZONTAL_VEL 12.0f    // units/sec max speed
#define ACCELERATION 42.0f          // units/sec² for speeding up
#define DECELERATION 58.0f          // units/sec² for slowing down
#define JUMP_FORCE 16.0f            // Initial vertical velocity
#define GRAVITY 42.0f               // units/sec² downward

// Combat mechanics
#define INVULNERABILITY_FRAMES 20U  // Frames of temporary invulnerability post-hit
#define PARRY_STARTUP_FRAMES 4U     // Frames before parry is active
#define PARRY_ACTIVE_FRAMES 8U      // Duration parry blocks hits
#define ATTACK_STARTUP_FRAMES 6U    // Frames of attack startup
#define ATTACK_ACTIVE_FRAMES 10U    // Duration hit can connect
#define STUN_FRAMES_ON_PARRY 10U    // Stun attacker when parried

// Reach/hitbox sizes (tiles)
#define BASE_SWORD_REACH 2.0f       // Units of reach
#define THROW_REACH 8.0f            // Throw range in tiles
#define THROW_SPEED 24.0f           // Horizontal sword projectile speed (tiles/sec)
#define SWORD_RECOVER_FRAMES 600U   // Frames before thrown sword auto-returns
#define PLAYER_WIDTH 1.0f
#define PLAYER_HEIGHT 2.0f

// Advanced movement/combat timings
#define PLAYER_RESPAWN_FRAMES 300U  // ~5s at 60 FPS
#define DOWNED_FRAMES 80U
#define ROLL_FRAMES 16U
#define CARTWHEEL_FRAMES 12U
#define ROLL_SPEED 14.0f
#define CARTWHEEL_SPEED 18.0f
#define CRAWL_SPEED 5.0f

// Rendering
#define TILE_SIZE_PX    32          // pixels per tile
#define WINDOW_W        1280
#define WINDOW_H        720
#define HUD_HEIGHT_PX   48

// Long-form arena layout
#define MAP_SEGMENT_TILES 40U
#define MAP_SIDE_SEGMENTS 2U
#define MAP_TOTAL_SEGMENTS (1U + (MAP_SIDE_SEGMENTS * 2U) + 2U)
#define MAP_MIDDLE_SEGMENT_INDEX (MAP_TOTAL_SEGMENTS / 2U)

// Default arena: enemy base | 2 themed maps | middle | 2 themed maps | enemy base
#define ARENA_DEFAULT_WIDTH  (MAP_SEGMENT_TILES * MAP_TOTAL_SEGMENTS)
#define ARENA_DEFAULT_HEIGHT  18U

// Segment flow
#define MAP_TRANSITION_FRAMES 30U
#define MAP_ENTRY_OFFSET 3.0f

// HUD hint duration after death (frames @ 60 FPS)
#define DEATH_POPUP_FRAMES 180U

// Match presentation
#define MATCH_COUNTDOWN_FRAMES 90U
#define ROUND_END_FRAMES 180U

#endif
