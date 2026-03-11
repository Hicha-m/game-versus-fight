#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum GameError {
	GAME_OK,
	GAME_ERROR_INVALID_ARGUMENT,
	GAME_ERROR_INVALID_STATE,
	GAME_ERROR_OUT_OF_MEMORY,
	GAME_ERROR_OUT_OF_BOUNDS,
	GAME_ERROR_NOT_FOUND,
	GAME_ERROR_UNSUPPORTED,
	GAME_ERROR_TIMEOUT,
	GAME_ERROR_IO,
	GAME_ERROR_INTERNAL
} GameError;

typedef enum PlayerId {
	PLAYER_ONE,
	PLAYER_TWO
} PlayerId;

typedef enum DifficultyLevel {
	DIFFICULTY_EASY,
	DIFFICULTY_NORMAL,
	DIFFICULTY_HARD,
	DIFFICULTY_EXPERT
} DifficultyLevel;

typedef enum GamePhase {
    GAME_PHASE_BOOT,
    GAME_PHASE_PRESS_START,
    GAME_PHASE_MAIN_MENU,
    GAME_PHASE_MODE_SELECT,
    GAME_PHASE_MATCH,
    GAME_PHASE_GAME_OVER,
    GAME_PHASE_QUIT
} GamePhase;


typedef enum MatchPhase {
	MATCH_PHASE_MENU,
	MATCH_PHASE_COUNTDOWN,
	MATCH_PHASE_FIGHT,
	MATCH_PHASE_ROUND_END,
	MATCH_PHASE_MATCH_END,
	MATCH_PHASE_PAUSED
} MatchPhase;

typedef enum FacingDirection {
	FACING_LEFT,
	FACING_RIGHT
} FacingDirection;

typedef enum SwordHeight {
	SWORD_HEIGHT_LOW,
	SWORD_HEIGHT_MID,
	SWORD_HEIGHT_HIGH
} SwordHeight;

typedef enum TileType {
	TILE_EMPTY,
	TILE_SOLID,
	TILE_PLATFORM,
	TILE_SPAWN_P1,
	TILE_SPAWN_P2,
	TILE_HAZARD
} TileType;

typedef enum ActionType {
	ACTION_NONE,
	ACTION_MOVE_LEFT,
	ACTION_MOVE_RIGHT,
	ACTION_JUMP,
	ACTION_ATTACK,
	ACTION_PARRY,
	ACTION_HEIGHT_CHANGE
} ActionType;

// integer grid coordinates
typedef struct Vec2i {
	int32_t x;
	int32_t y;
} Vec2i;

// floating‑point positions/velocities
typedef struct Vec2f {
	float x;
	float y;
} Vec2f;

typedef struct Action {
	ActionType type;
	SwordHeight sword_height;
	uint32_t issued_frame;
} Action;

typedef struct PlayerCommand {
	int8_t move_axis;
	bool jump;
	bool crouch;
	bool attack;
	bool parry;
	bool dash;
	SwordHeight target_height;
} PlayerCommand;

typedef struct FrameInput {
	PlayerCommand commands[2];
	bool pause_pressed;
	bool start_pressed;
	bool quit_requested;
} FrameInput;

typedef struct Arena {
	uint16_t width;
	uint16_t height;
	uint32_t seed;
	TileType *tiles;
} Arena;

typedef struct FighterState {
	Vec2f position;
	Vec2f velocity;
	FacingDirection facing;
	SwordHeight sword_height;
	bool alive;
	uint32_t momentum_frames;
	uint16_t successful_parries;
	bool grounded;
	uint8_t invulnerability_frames;
	uint8_t stun_frames;
} FighterState;

typedef struct CombatState {
	FighterState fighters[2];
	uint32_t round_time_frames;
	uint8_t score[2];
	bool duel_active;
} CombatState;

typedef struct GameConfig {
	uint16_t arena_width;
	uint16_t arena_height;
	uint32_t arena_seed;
	uint16_t max_round_time_seconds;
	DifficultyLevel ai_difficulty;
} GameConfig;

typedef struct GameState {
	GamePhase game_phase;     // état global
    MatchPhase match_phase;   // valide seulement si game_phase == GAME_PHASE_MATCH
	
	Arena arena;
	CombatState combat;

	DifficultyLevel ai_difficulty;
	uint64_t frame_index;
	uint32_t rng_state;
	bool running;
} GameState;

#endif
