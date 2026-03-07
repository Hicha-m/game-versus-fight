#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>

typedef int (*TestFunc)(void);

typedef struct TestCase {
	const char *name;
	TestFunc func;
} TestCase;

#define TEST_ASSERT_TRUE(cond)                                                    	\
do {                                                                          		\
	if (!(cond)) {                                                            		\
		fprintf(stderr, "Assertion failed: %s (%s:%d)\n", #cond, __FILE__,    		\
				__LINE__);                                                    		\
		return 1;                                                             		\
	}                                                                         		\
} while (0)																	  		\

#define TEST_ASSERT_EQ_INT(expected, actual)                                           \
	do {                                                                               \
		int _exp = (int)(expected);                                                    \
		int _act = (int)(actual);                                                      \
		if (_exp != _act) {                                                            \
			fprintf(stderr, "Assertion failed: expected %d got %d (%s:%d)\n", _exp,    \
			        _act, __FILE__, __LINE__);                                         \
			return 1;                                                                  \
		}                                                                              \
	} while (0)

int run_test_cases(const char *suite_name, const TestCase *cases, int count);

#endif