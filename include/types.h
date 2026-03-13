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

typedef enum GameMode {
	GAME_MODE_VERSUS,
	GAME_MODE_VS_AI
} GameMode;

typedef enum GamePhase {
    GAME_PHASE_BOOT,
    GAME_PHASE_PRESS_START,
    GAME_PHASE_MAIN_MENU,
	GAME_PHASE_MAP_MENU,
	GAME_PHASE_MAP_GENERATE,
	GAME_PHASE_MAP_RESULTS,
    GAME_PHASE_MODE_SELECT,
	GAME_PHASE_OPTIONS,
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

typedef enum ArchetypeType {
	ARCHETYPE_SMALL,
	ARCHETYPE_MEDIUM,
	ARCHETYPE_LARGE
} ArchetypeType;

typedef enum ActionType {
	ACTION_NONE,
	ACTION_MOVE_LEFT,
	ACTION_MOVE_RIGHT,
	ACTION_JUMP,
	ACTION_ATTACK,
	ACTION_PARRY,
	ACTION_HEIGHT_CHANGE
} ActionType;

typedef enum BindingAction {
	BIND_MOVE_LEFT,
	BIND_MOVE_RIGHT,
	BIND_JUMP,
	BIND_ATTACK,
	BIND_PARRY,
	BIND_STANCE_UP,
	BIND_STANCE_DOWN,
	BIND_ACTION_COUNT
} BindingAction;

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

typedef struct PlayerBindings {
	int32_t scancodes[BIND_ACTION_COUNT];
} PlayerBindings;

typedef struct FrameInput {
	PlayerCommand commands[2];
	bool pause_pressed;
	bool quit_requested;
	bool menu_up_pressed;
	bool menu_down_pressed;
	bool menu_left_pressed;
	bool menu_right_pressed;
	bool menu_confirm_pressed;
	bool menu_back_pressed;
	bool any_key_pressed;
	int32_t pressed_scancode;
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
	bool has_sword;
	bool sword_ready;
	uint16_t sword_recover_frames;
	bool alive;
	bool downed;
	uint16_t downed_frames;
	bool rolling;
	uint16_t roll_frames;
	bool cartwheeling;
	uint16_t cartwheel_frames;
	uint32_t momentum_frames;
	uint16_t successful_parries;
	bool attack_was_held;
	bool grounded;
	uint8_t drop_through_frames;
	uint8_t invulnerability_frames;
	uint8_t stun_frames;
} FighterState;

typedef struct CombatState {
	FighterState fighters[2];
	Vec2f sword_drop_position[2];
	Vec2f sword_flight_position[2];
	float sword_flight_velocity[2];
	uint32_t round_time_frames;
	uint16_t round_time_limit_seconds;
	uint8_t score[2];
	uint8_t kill_count[2];
	uint8_t death_count[2];
	uint8_t throw_kill_count[2];
	uint8_t thrust_kill_count[2];
	uint8_t disarm_count[2];
	uint8_t neck_snap_count[2];
	uint8_t zones_crossed[2];
	bool duel_active;
	bool has_priority;
	bool sword_on_ground[2];
	bool sword_in_flight[2];
	uint8_t block_spark_frames;
	bool segment_goal_mode;
	SwordHeight sword_flight_height[2];
	Vec2f block_spark_position;
	PlayerId priority_owner;
	uint16_t arena_width;            /* stored for goal-line win check */
	uint16_t respawn_frames[2];      /* countdown before a dead fighter returns */
	uint16_t death_popup_frames[2];  /* HUD hint countdown per player  */
} CombatState;

typedef struct GameConfig {
	uint16_t arena_width;
	uint16_t arena_height;
	uint32_t arena_seed;
	ArchetypeType arena_archetype;
	uint16_t max_round_time_seconds;
	DifficultyLevel ai_difficulty;
	GameMode game_mode;
	PlayerBindings bindings[2];
} GameConfig;

#define MENU_MAX_SAVED_MAPS 32
#define MENU_MAX_SAVED_MAP_NAME 80

typedef struct MenuState {
	uint8_t main_menu_index;
	uint8_t map_menu_index;
	uint8_t map_generate_index;
	uint8_t map_results_index;
	uint8_t mode_menu_index;
	uint8_t options_index;
	bool waiting_for_rebind;
	bool press_start_armed;
	bool use_saved_map;
	PlayerId rebind_player;
	BindingAction rebind_action;
	ArchetypeType generate_archetype;
	uint16_t generate_width;
	uint16_t generate_height;
	uint8_t generate_platform_density;
	uint8_t generate_hazard_count;
	uint8_t generate_hole_count;
	uint8_t generate_hole_max_width;
	bool generate_force_symmetry;
	uint32_t generate_seed;
	char selected_map_name[MENU_MAX_SAVED_MAP_NAME];
	char saved_map_names[MENU_MAX_SAVED_MAPS][MENU_MAX_SAVED_MAP_NAME];
	uint16_t saved_map_width[MENU_MAX_SAVED_MAPS];
	uint16_t saved_map_height[MENU_MAX_SAVED_MAPS];
	uint32_t saved_map_seed[MENU_MAX_SAVED_MAPS];
	ArchetypeType saved_map_archetype[MENU_MAX_SAVED_MAPS];
	uint8_t saved_map_filtered_indices[MENU_MAX_SAVED_MAPS];
	uint8_t saved_map_count;
	uint8_t saved_map_filtered_count;
	uint8_t map_results_page;
	bool map_results_search_edit;
	bool map_results_rename_edit;
	char map_results_query[MENU_MAX_SAVED_MAP_NAME];
	char map_results_edit_buffer[MENU_MAX_SAVED_MAP_NAME];
	Arena map_preview;
	char map_preview_name[MENU_MAX_SAVED_MAP_NAME];
	bool map_preview_loaded;
	char status_text[64];
	uint16_t status_frames;
} MenuState;

typedef struct GameState {
	GamePhase game_phase;    /* current top-level phase          */
	MatchPhase match_phase;  /* sub-phase during GAME_PHASE_MATCH */

	Arena arena;
	CombatState combat;
	GameConfig config;
	MenuState menu;

	DifficultyLevel ai_difficulty;
	GameMode game_mode;
	PlayerId round_winner;
	uint64_t frame_index;
	uint32_t rng_state;
	uint32_t phase_elapsed;  /* frames elapsed in current phase  */
	uint8_t active_segment;
	uint8_t pending_segment;
	uint16_t transition_frames;
	bool running;
} GameState;

#endif
