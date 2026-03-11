#include "ai.h"
#include "ai/difficulty.h"
#include "ai/evaluator.h"
#include "ai/minmax.h"
#include "ai/state.h"
#include "constants.h"
#include "test_common.h"

static GameState make_base_ai_state(TileType *tiles, uint16_t width, uint16_t height) {
	GameState state = {0};
	state.arena.width = width;
	state.arena.height = height;
	state.arena.tiles = tiles;
	state.combat.fighters[PLAYER_ONE].alive = true;
	state.combat.fighters[PLAYER_ONE].grounded = true;
	state.combat.fighters[PLAYER_ONE].sword_height = SWORD_HEIGHT_MID;
	state.combat.fighters[PLAYER_TWO].alive = true;
	state.combat.fighters[PLAYER_TWO].grounded = true;
	state.combat.fighters[PLAYER_TWO].sword_height = SWORD_HEIGHT_MID;
	return state;
}

static void set_fighter_position(GameState *state, PlayerId player, int32_t tile_x, int32_t tile_y) {
	state->combat.fighters[player].position.x = (float)(tile_x * ARENA_TILE_W);
	state->combat.fighters[player].position.y = (float)(tile_y * ARENA_TILE_H);
}

static int action_in_list(const AiActionList *list, ActionType type) {
	uint8_t idx = 0U;
	for (idx = 0U; idx < list->count; ++idx) {
		if (list->items[idx].action.type == type) {
			return 1;
		}
	}
	return 0;
}

static int same_action(const Action *left, const Action *right) {
	return left->type == right->type && left->sword_height == right->sword_height;
}

static int test_ai_choose_action_sets_defaults(void) {
	Action action = {0};
	GameError err = ai_choose_action(NULL, PLAYER_TWO, 1000U, &action);

	TEST_ASSERT_EQ_INT(GAME_ERROR_INVALID_ARGUMENT, err);
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
	TEST_ASSERT_EQ_INT(3, depth);
	return 0;
}

static int test_ai_difficulty_configs_match_story_calibration(void) {
	AiDifficultyConfig easy = {0};
	AiDifficultyConfig normal = {0};
	AiDifficultyConfig hard = {0};
	AiDifficultyConfig expert = {0};

	TEST_ASSERT_EQ_INT(GAME_OK, ai_difficulty_get_config(DIFFICULTY_EASY, &easy));
	TEST_ASSERT_EQ_INT(GAME_OK, ai_difficulty_get_config(DIFFICULTY_NORMAL, &normal));
	TEST_ASSERT_EQ_INT(GAME_OK, ai_difficulty_get_config(DIFFICULTY_HARD, &hard));
	TEST_ASSERT_EQ_INT(GAME_OK, ai_difficulty_get_config(DIFFICULTY_EXPERT, &expert));

	TEST_ASSERT_EQ_INT(1, easy.search_depth);
	TEST_ASSERT_EQ_INT(20, easy.randomness_percent);
	TEST_ASSERT_EQ_INT(2, normal.search_depth);
	TEST_ASSERT_EQ_INT(10, normal.randomness_percent);
	TEST_ASSERT_EQ_INT(3, hard.search_depth);
	TEST_ASSERT_EQ_INT(5, hard.randomness_percent);
	TEST_ASSERT_EQ_INT(3, expert.search_depth);
	TEST_ASSERT_EQ_INT(0, expert.randomness_percent);
	return 0;
}

static int test_ai_discrete_state_adjacent_no_obstacle(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiDiscreteState discrete = {0};

	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 3, 1);

	GameError err = ai_state_discretize(&state, PLAYER_TWO, &discrete);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(AI_LOCAL_DANGER_HIGH, discrete.danger);
	TEST_ASSERT_EQ_INT(AI_LOCAL_OBSTACLE_NONE, discrete.obstacle_between);
	TEST_ASSERT_EQ_INT(AI_LOCAL_ZONE_NEUTRAL, discrete.local_zone);
	TEST_ASSERT_EQ_INT(1, discrete.opponent_distance_x);
	return 0;
}

static int test_ai_discrete_state_obstacle_between_fighters(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiDiscreteState discrete = {0};

	tiles[1 * 6 + 2] = TILE_SOLID;
	set_fighter_position(&state, PLAYER_TWO, 1, 1);
	set_fighter_position(&state, PLAYER_ONE, 4, 1);

	GameError err = ai_state_discretize(&state, PLAYER_TWO, &discrete);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(AI_LOCAL_OBSTACLE_SOLID, discrete.obstacle_between);
	return 0;
}

static int test_ai_discrete_state_hazard_between_fighters(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiDiscreteState discrete = {0};

	tiles[1 * 6 + 3] = TILE_HAZARD;
	set_fighter_position(&state, PLAYER_TWO, 1, 1);
	set_fighter_position(&state, PLAYER_ONE, 5, 1);

	GameError err = ai_state_discretize(&state, PLAYER_TWO, &discrete);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(AI_LOCAL_OBSTACLE_HAZARD, discrete.obstacle_between);
	return 0;
}

static int test_ai_discrete_state_local_zone_from_actor_tile(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiDiscreteState discrete = {0};

	tiles[1 * 6 + 1] = TILE_HAZARD;
	set_fighter_position(&state, PLAYER_TWO, 1, 1);
	set_fighter_position(&state, PLAYER_ONE, 4, 1);

	GameError err = ai_state_discretize(&state, PLAYER_TWO, &discrete);

	TEST_ASSERT_EQ_INT(GAME_OK, err);
	TEST_ASSERT_EQ_INT(AI_LOCAL_ZONE_HAZARD, discrete.local_zone);
	return 0;
}

static int test_ai_discrete_state_invalid_arguments_and_state(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiDiscreteState discrete = {0};

	set_fighter_position(&state, PLAYER_TWO, 1, 1);
	set_fighter_position(&state, PLAYER_ONE, 4, 1);

	TEST_ASSERT_EQ_INT(GAME_ERROR_INVALID_ARGUMENT, ai_state_discretize(NULL, PLAYER_TWO, &discrete));
	TEST_ASSERT_EQ_INT(GAME_ERROR_INVALID_ARGUMENT, ai_state_discretize(&state, PLAYER_TWO, NULL));

	state.arena.tiles = NULL;
	TEST_ASSERT_EQ_INT(GAME_ERROR_INVALID_STATE, ai_state_discretize(&state, PLAYER_TWO, &discrete));
	state.arena.tiles = tiles;

	set_fighter_position(&state, PLAYER_TWO, 99, 1);
	TEST_ASSERT_EQ_INT(GAME_ERROR_OUT_OF_BOUNDS, ai_state_discretize(&state, PLAYER_TWO, &discrete));
	return 0;
}

static int test_ai_discrete_state_does_not_mutate_input_state(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiDiscreteState discrete = {0};
	GameState before = {0};

	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 5, 1);
	before = state;

	TEST_ASSERT_EQ_INT(GAME_OK, ai_state_discretize(&state, PLAYER_TWO, &discrete));
	TEST_ASSERT_EQ_INT(0, memcmp(&before, &state, sizeof(GameState)));
	return 0;
}

static int test_ai_evaluator_generates_deterministic_local_candidates(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiActionList first = {0};
	AiActionList second = {0};

	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 3, 1);

	TEST_ASSERT_EQ_INT(GAME_OK, ai_evaluator_generate_candidates(&state, PLAYER_TWO, &first));
	TEST_ASSERT_EQ_INT(GAME_OK, ai_evaluator_generate_candidates(&state, PLAYER_TWO, &second));
	TEST_ASSERT_TRUE(first.count > 0);
	TEST_ASSERT_TRUE(first.count <= AI_MAX_CANDIDATE_ACTIONS);
	TEST_ASSERT_TRUE(action_in_list(&first, ACTION_ATTACK));
	TEST_ASSERT_TRUE(action_in_list(&first, ACTION_PARRY));
	TEST_ASSERT_EQ_INT(0, memcmp(&first, &second, sizeof(AiActionList)));
	return 0;
}

static int test_ai_evaluator_filters_direct_engagement_across_hazard(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiActionList actions = {0};

	tiles[1 * 6 + 2] = TILE_HAZARD;
	set_fighter_position(&state, PLAYER_TWO, 1, 1);
	set_fighter_position(&state, PLAYER_ONE, 4, 1);

	TEST_ASSERT_EQ_INT(GAME_OK, ai_evaluator_generate_candidates(&state, PLAYER_TWO, &actions));
	TEST_ASSERT_TRUE(!action_in_list(&actions, ACTION_ATTACK));
	TEST_ASSERT_TRUE(!action_in_list(&actions, ACTION_MOVE_RIGHT));
	return 0;
}

static int test_ai_evaluator_leaf_score_rewards_terminal_attack(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	GameState attacked = {0};
	GameState retreated = {0};
	Action attack = {ACTION_ATTACK, SWORD_HEIGHT_MID, 0U};
	Action retreat = {ACTION_MOVE_LEFT, SWORD_HEIGHT_MID, 0U};
	int32_t attack_score = 0;
	int32_t retreat_score = 0;

	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 3, 1);

	TEST_ASSERT_EQ_INT(GAME_OK, ai_evaluator_apply_action(&state, PLAYER_TWO, &attack, &attacked));
	TEST_ASSERT_EQ_INT(GAME_OK, ai_evaluator_apply_action(&state, PLAYER_TWO, &retreat, &retreated));
	TEST_ASSERT_EQ_INT(GAME_OK, ai_evaluator_score_state(&attacked, PLAYER_TWO, &attack_score));
	TEST_ASSERT_EQ_INT(GAME_OK, ai_evaluator_score_state(&retreated, PLAYER_TWO, &retreat_score));
	TEST_ASSERT_TRUE(attack_score > retreat_score);
	return 0;
}

static int test_ai_minmax_depth_one_picks_immediate_kill(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiSearchResult result = {0};

	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 3, 1);

	TEST_ASSERT_EQ_INT(GAME_OK, ai_minmax_search(&state, PLAYER_TWO, PLAYER_TWO, 1U, 1000U, &result));
	TEST_ASSERT_EQ_INT(ACTION_ATTACK, result.action.type);
	return 0;
}

static int test_ai_minmax_can_beat_greedy_candidate_order(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	AiActionList actions = {0};
	AiSearchResult result = {0};

	tiles[1 * 6 + 2] = TILE_HAZARD;
	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 3, 1);

	TEST_ASSERT_EQ_INT(GAME_OK, ai_evaluator_generate_candidates(&state, PLAYER_TWO, &actions));
	TEST_ASSERT_TRUE(actions.count > 0);
	TEST_ASSERT_EQ_INT(GAME_OK, ai_minmax_search(&state, PLAYER_TWO, PLAYER_TWO, 3U, 1000U, &result));
	TEST_ASSERT_TRUE(!same_action(&actions.items[0].action, &result.action));
	return 0;
}

static int test_ai_minmax_reports_pruning_on_stable_order(void) {
	TileType tiles[21] = {0};
	GameState state = make_base_ai_state(tiles, 7, 3);
	AiSearchResult result = {0};

	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 4, 1);

	TEST_ASSERT_EQ_INT(GAME_OK, ai_minmax_search(&state, PLAYER_TWO, PLAYER_TWO, 3U, 1000U, &result));
	TEST_ASSERT_TRUE(result.explored_nodes > 0U);
	TEST_ASSERT_TRUE(result.pruned_nodes > 0U);
	return 0;
}

static int test_ai_choose_action_returns_safe_fallback_on_zero_budget(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	Action action = {0};

	state.ai_difficulty = DIFFICULTY_HARD;
	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 3, 1);

	TEST_ASSERT_EQ_INT(GAME_ERROR_TIMEOUT, ai_choose_action(&state, PLAYER_TWO, 0U, &action));
	TEST_ASSERT_TRUE(action.type == ACTION_PARRY || action.type == ACTION_MOVE_LEFT || action.type == ACTION_MOVE_RIGHT);
	return 0;
}

static int test_ai_choose_action_handles_deterministic_timeout_without_garbage_output(void) {
	TileType tiles[21] = {0};
	GameState state = make_base_ai_state(tiles, 7, 3);
	Action action = {0};

	state.ai_difficulty = DIFFICULTY_EXPERT;
	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 4, 1);
	ai_minmax_set_test_node_budget(1U);
	TEST_ASSERT_EQ_INT(GAME_ERROR_TIMEOUT, ai_choose_action(&state, PLAYER_TWO, 1000U, &action));
	ai_minmax_reset_test_node_budget();
	TEST_ASSERT_TRUE(action.type != ACTION_JUMP);
	return 0;
}

static int test_ai_get_action_facade_and_expert_are_stable(void) {
	TileType tiles[18] = {0};
	GameState left = make_base_ai_state(tiles, 6, 3);
	GameState right = make_base_ai_state(tiles, 6, 3);
	Action first = {0};
	Action second = {0};

	set_fighter_position(&left, PLAYER_TWO, 2, 1);
	set_fighter_position(&left, PLAYER_ONE, 3, 1);
	right = left;
	left.rng_state = 1U;
	right.rng_state = 99U;

	first = ai_get_action(&left, DIFFICULTY_EXPERT);
	second = ai_get_action(&right, DIFFICULTY_EXPERT);
	TEST_ASSERT_TRUE(same_action(&first, &second));
	return 0;
}

static int test_ai_easy_diverges_more_often_than_normal_and_hard(void) {
	TileType tiles[18] = {0};
	GameState state = make_base_ai_state(tiles, 6, 3);
	Action expert = {0};
	int easy_diff = 0;
	int normal_diff = 0;
	int hard_diff = 0;
	uint32_t seed = 0U;

	set_fighter_position(&state, PLAYER_TWO, 2, 1);
	set_fighter_position(&state, PLAYER_ONE, 3, 1);
	expert = ai_get_action(&state, DIFFICULTY_EXPERT);

	for (seed = 0U; seed < 64U; ++seed) {
		Action easy = {0};
		Action normal = {0};
		Action hard = {0};
		state.rng_state = seed;
		easy = ai_get_action(&state, DIFFICULTY_EASY);
		normal = ai_get_action(&state, DIFFICULTY_NORMAL);
		hard = ai_get_action(&state, DIFFICULTY_HARD);
		if (!same_action(&easy, &expert)) {
			easy_diff++;
		}
		if (!same_action(&normal, &expert)) {
			normal_diff++;
		}
		if (!same_action(&hard, &expert)) {
			hard_diff++;
		}
	}

	TEST_ASSERT_TRUE(easy_diff > 0);
	TEST_ASSERT_TRUE(easy_diff >= normal_diff);
	TEST_ASSERT_TRUE(normal_diff >= hard_diff);
	return 0;
}

int run_ai_tests(void) {
	TestCase cases[] = {
		{"choose_action_sets_defaults", test_ai_choose_action_sets_defaults},
		{"set_difficulty_updates_state", test_ai_set_difficulty_updates_state},
		{"evaluate_and_depth_mock_values", test_ai_evaluate_and_depth_mock_values},
		{"difficulty_configs_match_story_calibration", test_ai_difficulty_configs_match_story_calibration},
		{"discrete_state_adjacent_no_obstacle", test_ai_discrete_state_adjacent_no_obstacle},
		{"discrete_state_obstacle_between_fighters", test_ai_discrete_state_obstacle_between_fighters},
		{"discrete_state_hazard_between_fighters", test_ai_discrete_state_hazard_between_fighters},
		{"discrete_state_local_zone_from_actor_tile", test_ai_discrete_state_local_zone_from_actor_tile},
		{"discrete_state_invalid_arguments_and_state", test_ai_discrete_state_invalid_arguments_and_state},
		{"discrete_state_does_not_mutate_input_state", test_ai_discrete_state_does_not_mutate_input_state},
		{"evaluator_generates_deterministic_local_candidates", test_ai_evaluator_generates_deterministic_local_candidates},
		{"evaluator_filters_direct_engagement_across_hazard", test_ai_evaluator_filters_direct_engagement_across_hazard},
		{"evaluator_leaf_score_rewards_terminal_attack", test_ai_evaluator_leaf_score_rewards_terminal_attack},
		{"minmax_depth_one_picks_immediate_kill", test_ai_minmax_depth_one_picks_immediate_kill},
		{"minmax_can_beat_greedy_candidate_order", test_ai_minmax_can_beat_greedy_candidate_order},
		{"minmax_reports_pruning_on_stable_order", test_ai_minmax_reports_pruning_on_stable_order},
		{"choose_action_returns_safe_fallback_on_zero_budget", test_ai_choose_action_returns_safe_fallback_on_zero_budget},
		{"choose_action_handles_deterministic_timeout_without_garbage_output", test_ai_choose_action_handles_deterministic_timeout_without_garbage_output},
		{"get_action_facade_and_expert_are_stable", test_ai_get_action_facade_and_expert_are_stable},
		{"easy_diverges_more_often_than_normal_and_hard", test_ai_easy_diverges_more_often_than_normal_and_hard},
	};

	return run_test_cases("ai", cases, (int)(sizeof(cases) / sizeof(cases[0])));
}
