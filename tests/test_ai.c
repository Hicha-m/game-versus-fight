#include "ai.h"
#include "test_common.h"

static int test_ai_choose_action_sets_defaults(void) {
	Action action = {0};
	GameError err = ai_choose_action(NULL, PLAYER_TWO, 1000U, &action);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(ACTION_NONE, action.type);
	TEST_ASSERT_EQ_INT(SWORD_HEIGHT_MID, action.sword_height);
	TEST_ASSERT_EQ_INT(0, action.issued_frame);
	return 0;
}

static int test_ai_set_difficulty_updates_state(void) {
	GameState state = {0};
	GameError err = ai_set_difficulty(&state, DIFFICULTY_HARD);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(DIFFICULTY_HARD, state.ai_difficulty);
	return 0;
}

static int test_ai_evaluate_and_depth_mock_values(void) {
	int32_t score = ai_evaluate_state(NULL, PLAYER_ONE);
	uint8_t depth = ai_search_depth_for_difficulty(DIFFICULTY_EXPERT);

	TEST_ASSERT_EQ_INT(0, score);
	TEST_ASSERT_EQ_INT(1, depth);
	return 0;
}

int run_ai_tests(void) {
	TestCase cases[] = {
		{"choose_action_sets_defaults", test_ai_choose_action_sets_defaults},
		{"set_difficulty_updates_state", test_ai_set_difficulty_updates_state},
		{"evaluate_and_depth_mock_values", test_ai_evaluate_and_depth_mock_values},
	};

	return run_test_cases("ai", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
