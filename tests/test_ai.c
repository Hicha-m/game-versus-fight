#include "game/ai/ai.h"
#include "game/arena/arena.h"
#include "game/combat/combat.h"
#include "test_common.h"

static int test_ai_init_sets_expected_defaults(void) {
	AIController ai = {0};

	TEST_ASSERT_TRUE(ai_init(&ai));
	TEST_ASSERT_TRUE(ai.enabled);
	TEST_ASSERT_EQ_INT(AI_DIFFICULTY_MEDIUM, ai.difficulty);
	TEST_ASSERT_EQ_INT(AI_ALGO_MINIMAX_ALPHA_BETA, ai.algorithm);
	TEST_ASSERT_EQ_INT(3, ai.search_depth);
	return 0;
}

static int test_ai_set_difficulty_updates_search_depth(void) {
	AIController ai = {0};

	ai_init(&ai);
	ai_set_difficulty(&ai, AI_DIFFICULTY_HARD);
	TEST_ASSERT_EQ_INT(AI_DIFFICULTY_HARD, ai.difficulty);
	TEST_ASSERT_EQ_INT(4, ai.search_depth);

	ai_set_difficulty(&ai, AI_DIFFICULTY_EXPERT);
	TEST_ASSERT_EQ_INT(5, ai.search_depth);
	return 0;
}

static int test_ai_evaluate_state_null_is_zero(void) {
	AIHeuristicWeights w = {0};

	ai_default_weights(&w);
	TEST_ASSERT_EQ_INT(0, (int)ai_evaluate_state(NULL, &w));
	TEST_ASSERT_EQ_INT(0, (int)ai_evaluate_state((const AIDecisionState *)1, NULL));
	return 0;
}

static int test_ai_build_decision_state_and_think(void) {
	AIController ai = {0};
	Arena arena = {0};
	CombatState combat = {0};
	AIDecisionState state = {0};
	PlayerCommand cmd;

	arena_init(&arena);
	arena_build_default(&arena);
	combat_init(&combat);
	combat_reset_round(&combat, &arena);
	ai_init(&ai);

	combat.fighters[0].state.pos.x = 64.0f;
	combat.fighters[0].state.pos.y = 96.0f;
	combat.fighters[1].state.pos.x = 128.0f;
	combat.fighters[1].state.pos.y = 96.0f;

	ai_build_decision_state(&state, &arena, &combat, 0);
	TEST_ASSERT_EQ_INT(arena.current_room, state.current_room);
	TEST_ASSERT_EQ_INT(1, state.self_x_bucket);
	TEST_ASSERT_EQ_INT(2, state.enemy_x_bucket);

	i32 default_kills[MAX_PLAYERS] = {0, 0};
	cmd = ai_think(&ai, &arena, &combat, 0, FIXED_DT, default_kills);
	TEST_ASSERT_TRUE(cmd.move_x >= -1 && cmd.move_x <= 1);
	TEST_ASSERT_TRUE(cmd.target_sword_line >= SWORD_LINE_HIGH && cmd.target_sword_line <= SWORD_LINE_LOW);
	TEST_ASSERT_TRUE(ai.metrics.nodes_expanded > 0);
	return 0;
}

int run_ai_tests(void) {
	TestCase cases[] = {
		{"init_sets_expected_defaults", test_ai_init_sets_expected_defaults},
		{"set_difficulty_updates_search_depth", test_ai_set_difficulty_updates_search_depth},
		{"evaluate_state_null_is_zero", test_ai_evaluate_state_null_is_zero},
		{"build_decision_state_and_think", test_ai_build_decision_state_and_think},
	};

	return run_test_cases("ai", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
