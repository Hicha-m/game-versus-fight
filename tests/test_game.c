#include "game.h"
#include "arena.h"
#include "constants.h"
#include "game/match.h"
#include "test_common.h"

#include <SDL3/SDL.h>
#include <stdio.h>

static int test_game_create_initializes_state(void) {
	GameState state = {0};
	GameConfig config = {0};

	GameError err = game_create(&config, &state);
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_TRUE(state.running);
	TEST_ASSERT_EQ_INT(0, state.frame_index);
	return 0;
}

static int test_game_update_increments_frame(void) {
	GameState state = {0};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
	TEST_ASSERT_EQ_INT(1, state.frame_index);
	return 0;
}

static void setup_duel_positions(GameState *state) {
	state->combat.fighters[PLAYER_ONE].position.x = 136.0f;
	state->combat.fighters[PLAYER_ONE].position.y = 15.0f;
	state->combat.fighters[PLAYER_ONE].facing = FACING_RIGHT;

	state->combat.fighters[PLAYER_TWO].position.x = 137.0f;
	state->combat.fighters[PLAYER_TWO].position.y = 15.0f;
	state->combat.fighters[PLAYER_TWO].facing = FACING_LEFT;
}

static int test_press_start_requires_arm_delay_then_accepts_any_key(void) {
	GameState state = {0};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(NULL, &state));

	input.any_key_pressed = true;
	TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
	TEST_ASSERT_EQ_INT(GAME_PHASE_PRESS_START, state.game_phase);

	for (uint32_t i = 0; i < 20U; ++i) {
		TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
		TEST_ASSERT_EQ_INT(GAME_PHASE_PRESS_START, state.game_phase);
	}

	TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
	TEST_ASSERT_EQ_INT(GAME_PHASE_MAIN_MENU, state.game_phase);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_same_height_thrusts_auto_block(void) {
	GameState state = {0};
	GameConfig config = {
		.arena_width = ARENA_DEFAULT_WIDTH,
		.arena_height = ARENA_DEFAULT_HEIGHT,
		.game_mode = GAME_MODE_VERSUS,
		.ai_difficulty = DIFFICULTY_NORMAL,
		.max_round_time_seconds = DEFAULT_ROUND_TIME_SECONDS,
	};
	FrameInput input = {0};
	const SwordHeight heights[] = {SWORD_HEIGHT_HIGH, SWORD_HEIGHT_MID, SWORD_HEIGHT_LOW};

	for (size_t i = 0; i < sizeof(heights) / sizeof(heights[0]); ++i) {
		TEST_ASSERT_EQ_INT(GAME_OK, game_create(&config, &state));
		state.rng_state = (uint32_t)(40U + i);
		TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
		state.match_phase = MATCH_PHASE_FIGHT;

		setup_duel_positions(&state);
		state.combat.fighters[PLAYER_ONE].sword_height = heights[i];
		state.combat.fighters[PLAYER_TWO].sword_height = heights[i];

		input = (FrameInput){0};
		input.commands[PLAYER_ONE].target_height = heights[i];
		input.commands[PLAYER_TWO].target_height = heights[i];
		input.commands[PLAYER_ONE].attack = true;
		input.commands[PLAYER_TWO].attack = true;

		TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
		TEST_ASSERT_TRUE(state.combat.fighters[PLAYER_ONE].alive);
		TEST_ASSERT_TRUE(state.combat.fighters[PLAYER_TWO].alive);
		TEST_ASSERT_TRUE(state.combat.fighters[PLAYER_ONE].has_sword);
		TEST_ASSERT_TRUE(state.combat.fighters[PLAYER_TWO].has_sword);
		TEST_ASSERT_EQ_INT(0, state.combat.disarm_count[PLAYER_ONE]);
		TEST_ASSERT_EQ_INT(0, state.combat.disarm_count[PLAYER_TWO]);

		TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	}

	return 0;
}

static int test_mismatched_simultaneous_thrusts_double_ko(void) {
	GameState state = {0};
	GameConfig config = {
		.arena_width = ARENA_DEFAULT_WIDTH,
		.arena_height = ARENA_DEFAULT_HEIGHT,
		.game_mode = GAME_MODE_VERSUS,
		.ai_difficulty = DIFFICULTY_NORMAL,
		.max_round_time_seconds = DEFAULT_ROUND_TIME_SECONDS,
	};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(&config, &state));
	state.rng_state = 96;
	TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
	state.match_phase = MATCH_PHASE_FIGHT;

	setup_duel_positions(&state);
	state.combat.fighters[PLAYER_ONE].sword_height = SWORD_HEIGHT_HIGH;
	state.combat.fighters[PLAYER_TWO].sword_height = SWORD_HEIGHT_LOW;

	input.commands[PLAYER_ONE].target_height = SWORD_HEIGHT_HIGH;
	input.commands[PLAYER_TWO].target_height = SWORD_HEIGHT_LOW;
	input.commands[PLAYER_ONE].attack = true;
	input.commands[PLAYER_TWO].attack = true;

	TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
	TEST_ASSERT_TRUE(!state.combat.fighters[PLAYER_ONE].alive);
	TEST_ASSERT_TRUE(!state.combat.fighters[PLAYER_TWO].alive);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_mismatched_height_enemy_thrust_kills_you(void) {
	GameState state = {0};
	GameConfig config = {
		.arena_width = ARENA_DEFAULT_WIDTH,
		.arena_height = ARENA_DEFAULT_HEIGHT,
		.game_mode = GAME_MODE_VERSUS,
		.ai_difficulty = DIFFICULTY_NORMAL,
		.max_round_time_seconds = DEFAULT_ROUND_TIME_SECONDS,
	};
	FrameInput input = {0};
	const SwordHeight pairs[][2] = {
		{SWORD_HEIGHT_HIGH, SWORD_HEIGHT_MID},
		{SWORD_HEIGHT_HIGH, SWORD_HEIGHT_LOW},
		{SWORD_HEIGHT_MID, SWORD_HEIGHT_HIGH},
		{SWORD_HEIGHT_MID, SWORD_HEIGHT_LOW},
		{SWORD_HEIGHT_LOW, SWORD_HEIGHT_HIGH},
		{SWORD_HEIGHT_LOW, SWORD_HEIGHT_MID},
	};

	for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); ++i) {
		SwordHeight your_height = pairs[i][0];
		SwordHeight enemy_height = pairs[i][1];

		TEST_ASSERT_EQ_INT(GAME_OK, game_create(&config, &state));
		state.rng_state = (uint32_t)(70U + i);
		TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
		state.match_phase = MATCH_PHASE_FIGHT;

		setup_duel_positions(&state);
		state.combat.fighters[PLAYER_ONE].sword_height = your_height;
		state.combat.fighters[PLAYER_TWO].sword_height = enemy_height;

		input = (FrameInput){0};
		input.commands[PLAYER_ONE].target_height = your_height;
		input.commands[PLAYER_TWO].target_height = enemy_height;
		input.commands[PLAYER_TWO].attack = true;

		TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
		TEST_ASSERT_TRUE(!state.combat.fighters[PLAYER_ONE].alive);
		TEST_ASSERT_TRUE(state.combat.fighters[PLAYER_TWO].alive);

		TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	}

	return 0;
}

static int test_game_destroy_and_error_string(void) {
	GameState state = {0};
	state.running = true;

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	TEST_ASSERT_TRUE(!state.running);
	/* the string table should give the right text for each enum value */
	TEST_ASSERT_EQ_STRING("ok", game_error_string(GAME_OK));
	TEST_ASSERT_EQ_STRING("invalid argument",
			     game_error_string(GAME_ERROR_INVALID_ARGUMENT));
	/* an invalid enum (out of bounds) falls back to the unknown message */
	TEST_ASSERT_EQ_STRING("unknown error",
			     game_error_string((GameError)0xdead));
	return 0;
}

static int test_match_kill_then_respawn(void) {
	GameState state = {0};
	GameConfig config = {
		.arena_width = ARENA_DEFAULT_WIDTH,
		.arena_height = ARENA_DEFAULT_HEIGHT,
		.game_mode = GAME_MODE_VERSUS,
		.ai_difficulty = DIFFICULTY_NORMAL,
	};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(&config, &state));
	state.rng_state = 1;
	TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
	state.match_phase = MATCH_PHASE_FIGHT;

	state.combat.fighters[PLAYER_ONE].position.x = 136.0f;
	state.combat.fighters[PLAYER_ONE].position.y = 15.0f;
	state.combat.fighters[PLAYER_ONE].facing = FACING_RIGHT;
	state.combat.fighters[PLAYER_ONE].sword_height = SWORD_HEIGHT_HIGH;

	state.combat.fighters[PLAYER_TWO].position.x = 137.0f;
	state.combat.fighters[PLAYER_TWO].position.y = 15.0f;
	state.combat.fighters[PLAYER_TWO].facing = FACING_RIGHT;
	state.combat.fighters[PLAYER_TWO].sword_height = SWORD_HEIGHT_LOW;

	input.commands[PLAYER_ONE].target_height = SWORD_HEIGHT_HIGH;
	input.commands[PLAYER_TWO].target_height = SWORD_HEIGHT_LOW;
	input.commands[PLAYER_ONE].attack = true;

	TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
	TEST_ASSERT_TRUE(!state.combat.fighters[PLAYER_TWO].alive);
	TEST_ASSERT_EQ_INT(PLAYER_RESPAWN_FRAMES, state.combat.respawn_frames[PLAYER_TWO]);

	input = (FrameInput){0};
	input.commands[PLAYER_ONE].target_height = state.combat.fighters[PLAYER_ONE].sword_height;
	input.commands[PLAYER_TWO].target_height = SWORD_HEIGHT_MID;
	for (uint32_t i = 0; i < PLAYER_RESPAWN_FRAMES; ++i) {
		TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
	}

	TEST_ASSERT_TRUE(state.combat.fighters[PLAYER_TWO].alive);
	TEST_ASSERT_EQ_INT(0, state.combat.respawn_frames[PLAYER_TWO]);
	TEST_ASSERT_EQ_INT(123, (int)state.combat.fighters[PLAYER_TWO].position.x);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_options_rebind_does_not_crash(void) {
	GameState state = {0};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(NULL, &state));
	state.game_phase = GAME_PHASE_OPTIONS;
	state.menu.options_index = 0; /* P1 Move Left */

	input.menu_confirm_pressed = true;
	TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
	TEST_ASSERT_TRUE(state.menu.waiting_for_rebind);

	input = (FrameInput){0};
	input.pressed_scancode = SDL_SCANCODE_Z;
	TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
	TEST_ASSERT_TRUE(!state.menu.waiting_for_rebind);
	TEST_ASSERT_EQ_INT(SDL_SCANCODE_Z, state.config.bindings[PLAYER_ONE].scancodes[BIND_MOVE_LEFT]);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_match_allows_sword_pickup_while_opponent_is_dead(void) {
	GameState state = {0};
	GameConfig config = {
		.arena_width = ARENA_DEFAULT_WIDTH,
		.arena_height = ARENA_DEFAULT_HEIGHT,
		.game_mode = GAME_MODE_VERSUS,
		.ai_difficulty = DIFFICULTY_NORMAL,
		.max_round_time_seconds = DEFAULT_ROUND_TIME_SECONDS,
	};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(&config, &state));
	state.rng_state = 2;
	TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
	state.match_phase = MATCH_PHASE_FIGHT;

	state.combat.fighters[PLAYER_ONE].has_sword = false;
	state.combat.fighters[PLAYER_ONE].sword_recover_frames = 120;
	state.combat.fighters[PLAYER_ONE].position.x = 120.0f;
	state.combat.fighters[PLAYER_ONE].position.y = 15.0f;
	state.combat.fighters[PLAYER_TWO].alive = false;
	state.combat.respawn_frames[PLAYER_TWO] = PLAYER_RESPAWN_FRAMES;
	state.combat.sword_on_ground[PLAYER_ONE] = true;
	state.combat.sword_drop_position[PLAYER_ONE].x = 120.4f;
	state.combat.sword_drop_position[PLAYER_ONE].y = 15.4f;

	input.commands[PLAYER_ONE].crouch = true;
	input.commands[PLAYER_ONE].target_height = SWORD_HEIGHT_LOW;

	TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
	TEST_ASSERT_TRUE(state.combat.fighters[PLAYER_ONE].has_sword);
	TEST_ASSERT_TRUE(!state.combat.sword_on_ground[PLAYER_ONE]);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_match_kill_drops_victim_sword(void) {
	GameState state = {0};
	GameConfig config = {
		.arena_width = ARENA_DEFAULT_WIDTH,
		.arena_height = ARENA_DEFAULT_HEIGHT,
		.game_mode = GAME_MODE_VERSUS,
		.ai_difficulty = DIFFICULTY_NORMAL,
		.max_round_time_seconds = DEFAULT_ROUND_TIME_SECONDS,
	};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(&config, &state));
	state.rng_state = 3;
	TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
	state.match_phase = MATCH_PHASE_FIGHT;

	state.combat.fighters[PLAYER_ONE].position.x = 136.0f;
	state.combat.fighters[PLAYER_ONE].position.y = 15.0f;
	state.combat.fighters[PLAYER_ONE].facing = FACING_RIGHT;
	state.combat.fighters[PLAYER_ONE].sword_height = SWORD_HEIGHT_HIGH;

	state.combat.fighters[PLAYER_TWO].position.x = 137.0f;
	state.combat.fighters[PLAYER_TWO].position.y = 15.0f;
	state.combat.fighters[PLAYER_TWO].facing = FACING_RIGHT;
	state.combat.fighters[PLAYER_TWO].sword_height = SWORD_HEIGHT_LOW;

	input.commands[PLAYER_ONE].target_height = SWORD_HEIGHT_HIGH;
	input.commands[PLAYER_TWO].target_height = SWORD_HEIGHT_LOW;
	input.commands[PLAYER_ONE].attack = true;

	TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
	TEST_ASSERT_TRUE(!state.combat.fighters[PLAYER_TWO].alive);
	TEST_ASSERT_TRUE(state.combat.sword_on_ground[PLAYER_TWO]);
	TEST_ASSERT_TRUE(!state.combat.fighters[PLAYER_TWO].has_sword);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_mode_menu_can_change_round_time(void) {
	GameState state = {0};
	FrameInput input = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(NULL, &state));
	state.game_phase = GAME_PHASE_MODE_SELECT;
	state.menu.mode_menu_index = 2;
	state.config.max_round_time_seconds = 120;

	input.menu_right_pressed = true;
	TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
	TEST_ASSERT_EQ_INT(150, state.config.max_round_time_seconds);

	input = (FrameInput){0};
	input.menu_confirm_pressed = true;
	TEST_ASSERT_EQ_INT(GAME_OK, game_update(&state, &input));
	TEST_ASSERT_EQ_INT(180, state.config.max_round_time_seconds);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_small_saved_map_does_not_end_round_immediately(void) {
	GameState state = {0};
	Arena generated = {0};
	ArenaGenerationOptions options;
	FrameInput input = {0};
	const char *name = "test_small_start_map";

	arena_generation_options_defaults(&options, ARCHETYPE_SMALL);
	TEST_ASSERT_EQ_INT(GAME_OK, arena_generate_with_options(&generated, 96U, 14U, 777U, &options));
	TEST_ASSERT_EQ_INT(GAME_OK, arena_save_to_file(&generated, name, ARCHETYPE_SMALL));

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(NULL, &state));
	snprintf(state.menu.selected_map_name, sizeof(state.menu.selected_map_name), "%s", name);
	state.menu.use_saved_map = true;
	TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
	state.match_phase = MATCH_PHASE_FIGHT;

	TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
	TEST_ASSERT_TRUE(state.match_phase == MATCH_PHASE_FIGHT);

	TEST_ASSERT_EQ_INT(GAME_OK, arena_delete_saved_map(name));
	arena_destroy(&generated);
	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_round_timeout_uses_configured_limit(void) {
	GameState state = {0};
	GameConfig config = {
		.arena_width = ARENA_DEFAULT_WIDTH,
		.arena_height = ARENA_DEFAULT_HEIGHT,
		.game_mode = GAME_MODE_VERSUS,
		.ai_difficulty = DIFFICULTY_NORMAL,
		.max_round_time_seconds = MIN_ROUND_TIME_SECONDS,
	};
	FrameInput input = {0};
	uint32_t frames_to_timeout = ((uint32_t)MIN_ROUND_TIME_SECONDS * 1000U) / FIXED_TIMESTEP_MS;

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(&config, &state));
	state.rng_state = 4;
	TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
	state.match_phase = MATCH_PHASE_FIGHT;

	for (uint32_t i = 0; i < frames_to_timeout; ++i) {
		TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
	}

	TEST_ASSERT_EQ_INT(MATCH_PHASE_ROUND_END, state.match_phase);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

static int test_priority_owner_can_advance_segment_while_both_players_are_alive(void) {
	GameState state = {0};
	GameConfig config = {
		.arena_width = ARENA_DEFAULT_WIDTH,
		.arena_height = ARENA_DEFAULT_HEIGHT,
		.game_mode = GAME_MODE_VERSUS,
		.ai_difficulty = DIFFICULTY_NORMAL,
		.max_round_time_seconds = DEFAULT_ROUND_TIME_SECONDS,
	};
	FrameInput input = {0};
	float left = (float)(MAP_MIDDLE_SEGMENT_INDEX * MAP_SEGMENT_TILES);

	TEST_ASSERT_EQ_INT(GAME_OK, game_create(&config, &state));
	state.rng_state = 5;
	TEST_ASSERT_EQ_INT(GAME_OK, match_start(&state));
	state.match_phase = MATCH_PHASE_FIGHT;
	state.combat.has_priority = true;
	state.combat.priority_owner = PLAYER_ONE;
	state.combat.fighters[PLAYER_ONE].position.x = left - 0.25f;
	state.combat.fighters[PLAYER_ONE].position.y = 15.0f;
	state.combat.fighters[PLAYER_TWO].position.x = left + 8.0f;
	state.combat.fighters[PLAYER_TWO].position.y = 15.0f;

	TEST_ASSERT_EQ_INT(GAME_OK, match_update(&state, &input));
	TEST_ASSERT_EQ_INT(MAP_TRANSITION_FRAMES, state.transition_frames);
	TEST_ASSERT_EQ_INT(MAP_MIDDLE_SEGMENT_INDEX - 1U, state.pending_segment);

	TEST_ASSERT_EQ_INT(GAME_OK, game_destroy(&state));
	return 0;
}

int run_game_tests(void) {
	TestCase cases[] = {
		{"create_initializes_state", test_game_create_initializes_state},
		{"update_increments_frame", test_game_update_increments_frame},
		{"press_start_requires_arm_delay_then_accepts_any_key", test_press_start_requires_arm_delay_then_accepts_any_key},
		{"same_height_thrusts_auto_block", test_same_height_thrusts_auto_block},
		{"mismatched_simultaneous_thrusts_double_ko", test_mismatched_simultaneous_thrusts_double_ko},
		{"mismatched_height_enemy_thrust_kills_you", test_mismatched_height_enemy_thrust_kills_you},
		{"options_rebind_does_not_crash", test_options_rebind_does_not_crash},
		{"match_kill_then_respawn", test_match_kill_then_respawn},
		{"match_allows_sword_pickup_while_opponent_is_dead", test_match_allows_sword_pickup_while_opponent_is_dead},
		{"match_kill_drops_victim_sword", test_match_kill_drops_victim_sword},
		{"mode_menu_can_change_round_time", test_mode_menu_can_change_round_time},
		{"small_saved_map_does_not_end_round_immediately", test_small_saved_map_does_not_end_round_immediately},
		{"round_timeout_uses_configured_limit", test_round_timeout_uses_configured_limit},
		{"priority_owner_can_advance_segment_while_both_players_are_alive", test_priority_owner_can_advance_segment_while_both_players_are_alive},
		{"destroy_and_error_string", test_game_destroy_and_error_string},
	};

	return run_test_cases("game", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
