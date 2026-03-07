#include "test_common.h"

int run_ai_tests(void);
int run_arena_tests(void);
int run_combat_tests(void);
int run_engine_tests(void);
int run_game_tests(void);

int run_test_cases(const char *suite_name, const TestCase *cases, int count) {
	int failures = 0;

	for (int i = 0; i < count; ++i) {
		int result = cases[i].func();
		if (result == 0) {
			printf("[PASS] %s::%s\n", suite_name, cases[i].name);
		} else {
			printf("[FAIL] %s::%s\n", suite_name, cases[i].name);
			failures++;
		}
	}

	return failures;
}

int main(void) {
	int failures = 0;

	failures += run_ai_tests();
	failures += run_arena_tests();
	failures += run_combat_tests();
	failures += run_engine_tests();
	failures += run_game_tests();

	if (failures == 0) {
		printf("All tests passed.\n");
		return 0;
	}

	printf("%d test(s) failed.\n", failures);
	return 1;
}