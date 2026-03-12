#include "combat.h"
#include "test_common.h"

static int test_combat_init_sets_defaults(void) {
	CombatState combat = {0};
	Arena arena = {0};
	// Need a minimal valid arena for combat_init
	arena.width = 32;
	arena.height = 16;
	GameError err = combat_init(&combat, &arena, NULL);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_TRUE(combat.duel_active);
	TEST_ASSERT_EQ_INT(0, combat.round_time_frames);
	TEST_ASSERT_EQ_INT(0, combat.score[0]);
	TEST_ASSERT_EQ_INT(0, combat.score[1]);
	return 0;
}

static int test_combat_step_increments_round_time(void) {
	CombatState combat = {0};
	combat.round_time_frames = 10;
	combat.duel_active = true;
	
	Arena arena = {0};
	arena.width = 32;
	arena.height = 16;
	
	GameError err = combat_step(&combat, &arena, 16U);
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(11, combat.round_time_frames);
	return 0;
}

static int test_combat_is_round_over_mock_behavior(void) {
	CombatState combat = {0};
	combat.duel_active = false;  // Default state with no active fight
	PlayerId winner = PLAYER_TWO;

	bool over = combat_is_round_over(&combat, &winner);
	TEST_ASSERT_TRUE(!over);
	return 0;
}

int run_combat_tests(void) {
	TestCase cases[] = {
		{"init_sets_defaults", test_combat_init_sets_defaults},
		{"step_increments_round_time", test_combat_step_increments_round_time},
		{"is_round_over_mock_behavior", test_combat_is_round_over_mock_behavior},
	};

	return run_test_cases("combat", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
