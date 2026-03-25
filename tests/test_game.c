#include "game/game.h"
#include "test_common.h"

static int test_game_init_initializes_match_state(void) {
	Game game = {0};

	TEST_ASSERT_TRUE(game_init(&game));
	TEST_ASSERT_EQ_INT(GAME_PHASE_MATCH, game.phase);
	TEST_ASSERT_EQ_INT(CONTROLLER_HUMAN, game.combat.fighters[0].controller.type);
	TEST_ASSERT_EQ_INT(CONTROLLER_AI, game.combat.fighters[1].controller.type);
	TEST_ASSERT_TRUE(game.combat.fighters[0].state.alive);
	TEST_ASSERT_TRUE(game.combat.fighters[1].state.alive);
	game_shutdown(&game);
	return 0;
}

static int test_game_update_pause_toggles_phase(void) {
	Game game = {0};
	FrameInput input = {0};

	TEST_ASSERT_TRUE(game_init(&game));

	input.pause.pressed = true;
	game_update(&game, &input, FIXED_DT);
	TEST_ASSERT_EQ_INT(GAME_PHASE_PAUSED, game.phase);

	input.pause.pressed = true;
	game_update(&game, &input, FIXED_DT);
	TEST_ASSERT_EQ_INT(GAME_PHASE_MATCH, game.phase);

	game_shutdown(&game);
	return 0;
}

static int test_game_update_handles_round_end_stats(void) {
	Game game = {0};
	FrameInput input = {0};

	TEST_ASSERT_TRUE(game_init(&game));

	game.combat.round_over = true;
	game.combat.winner_index = 1;
	game.combat.fighters[0].state.has_sword = false;

	game_update(&game, &input, FIXED_DT);

	TEST_ASSERT_EQ_INT(1, game.match_stats.rounds_played);
	TEST_ASSERT_EQ_INT(1, game.match_stats.wins[1]);
	TEST_ASSERT_EQ_INT(1, game.match_stats.kills[1]);
	TEST_ASSERT_EQ_INT(1, game.match_stats.disarms[0]);

	/* Round end should trigger a reset for the next round. */
	TEST_ASSERT_TRUE(!game.combat.round_over);
	TEST_ASSERT_EQ_INT(-1, game.combat.winner_index);

	game_shutdown(&game);
	return 0;
}

static int test_game_update_ignores_null_input(void) {
	Game game = {0};
	Game snapshot = {0};
	TEST_ASSERT_TRUE(game_init(&game));

	/* Seed non-default values to ensure NULL input does not mutate gameplay state. */
	game.phase = GAME_PHASE_PAUSED;
	game.arena.current_room = 2;
	game.combat.round_over = true;
	game.combat.winner_index = 1;
	game.combat.fighters[0].state.pos.x = 123.0f;
	game.combat.fighters[0].state.pos.y = 45.0f;
	game.combat.fighters[0].state.alive = false;
	game.combat.fighters[1].state.attack_cooldown = 0.75f;
	game.match_stats.rounds_played = 7;
	game.match_stats.wins[0] = 3;
	game.match_stats.kills[1] = 2;

	snapshot = game;

	game_update(&game, NULL, FIXED_DT);

	TEST_ASSERT_EQ_INT(snapshot.phase, game.phase);
	TEST_ASSERT_EQ_INT(snapshot.arena.current_room, game.arena.current_room);
	TEST_ASSERT_EQ_INT(snapshot.combat.round_over, game.combat.round_over);
	TEST_ASSERT_EQ_INT(snapshot.combat.winner_index, game.combat.winner_index);
	TEST_ASSERT_EQ_INT(snapshot.combat.fighters[0].state.alive,
	                   game.combat.fighters[0].state.alive);
	TEST_ASSERT_TRUE(memcmp(&snapshot.combat.fighters[0].state.pos,
	                        &game.combat.fighters[0].state.pos,
	                        sizeof(snapshot.combat.fighters[0].state.pos)) == 0);
	TEST_ASSERT_TRUE(snapshot.combat.fighters[1].state.attack_cooldown ==
	                 game.combat.fighters[1].state.attack_cooldown);
	TEST_ASSERT_EQ_INT(snapshot.match_stats.rounds_played,
	                   game.match_stats.rounds_played);
	TEST_ASSERT_EQ_INT(snapshot.match_stats.wins[0], game.match_stats.wins[0]);
	TEST_ASSERT_EQ_INT(snapshot.match_stats.kills[1], game.match_stats.kills[1]);
	game_shutdown(&game);
	return 0;
}

int run_game_tests(void) {
	TestCase cases[] = {
		{"init_initializes_match_state", test_game_init_initializes_match_state},
		{"update_pause_toggles_phase", test_game_update_pause_toggles_phase},
		{"update_handles_round_end_stats", test_game_update_handles_round_end_stats},
		{"update_ignores_null_input", test_game_update_ignores_null_input},
	};

	return run_test_cases("game", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
