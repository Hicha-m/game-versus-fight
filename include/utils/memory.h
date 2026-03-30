#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include "core/types.h"

typedef struct MemoryStats {
    u64 allocation_count;
    u64 free_count;
    u64 bytes_current;
    u64 bytes_peak;
} MemoryStats;

void memory_init(void);
void memory_shutdown(void);

void* memory_alloc(size_t size, const char* file, int line);
void memory_free(void* ptr);

MemoryStats memory_get_stats(void);
bool memory_has_leaks(void);

#define MEM_ALLOC(size) memory_alloc((size), __FILE__, __LINE__)

#endif
