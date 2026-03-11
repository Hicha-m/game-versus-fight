#include "combat.h"
#include "test_common.h"

static int test_combat_init_sets_defaults(void) {
	CombatState combat = {0};
	GameError err = combat_init(&combat, NULL, NULL);

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

	GameError err = combat_step(&combat, NULL, 16U);
	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(11, combat.round_time_frames);
	return 0;
}

static int test_combat_is_round_over_both_alive(void) {
	CombatState combat = {0};
	combat.duel_active = true;
	combat.fighters[PLAYER_ONE].alive = true;
	combat.fighters[PLAYER_TWO].alive = true;

	PlayerId winner = PLAYER_TWO;
	bool over = combat_is_round_over(&combat, &winner);
	TEST_ASSERT_TRUE(!over);
	return 0;
}

int run_combat_tests(void) {
	TestCase cases[] = {
		{"init_sets_defaults", test_combat_init_sets_defaults},
		{"step_increments_round_time", test_combat_step_increments_round_time},
		{"is_round_over_both_alive", test_combat_is_round_over_both_alive},
	};

	return run_test_cases("combat", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
