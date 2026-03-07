#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

typedef enum LogLevel {
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR
} LogLevel;

// memory alocation // to track each memory leak (malloc, free)
void *br_alloc(size_t size);
void *br_calloc(size_t count, size_t size); // init memory with 0
void *br_realloc(void *ptr, size_t size);
void br_free(void *ptr);

void log_set_level(LogLevel level);
void log_write(LogLevel level, const char *message);

// have value resize in interval.
float math_clampf(float value, float min_value, float max_value);
int32_t math_clampi(int32_t value, int32_t min_value, int32_t max_value);

#endif
