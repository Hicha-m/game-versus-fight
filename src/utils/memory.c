#include <stdlib.h>

#include "utils/memory.h"

typedef struct MemoryHeader {
	size_t size;
} MemoryHeader;

static MemoryStats g_memory_stats;

void memory_init(void)
{
	g_memory_stats.allocation_count = 0;
	g_memory_stats.free_count = 0;
	g_memory_stats.bytes_current = 0;
	g_memory_stats.bytes_peak = 0;
}

void memory_shutdown(void)
{
}

void* memory_alloc(size_t size, const char* file, int line)
{
	MemoryHeader* header;
	size_t total_size;

	(void)file;
	(void)line;

	if (size == 0) {
		return NULL;
	}

	total_size = sizeof(MemoryHeader) + size;
	header = (MemoryHeader*)malloc(total_size);
	if (!header) {
		return NULL;
	}

	header->size = size;

	g_memory_stats.allocation_count++;
	g_memory_stats.bytes_current += (u64)size;
	if (g_memory_stats.bytes_current > g_memory_stats.bytes_peak) {
		g_memory_stats.bytes_peak = g_memory_stats.bytes_current;
	}

	return (void*)(header + 1);
}

void memory_free(void* ptr)
{
	MemoryHeader* header;

	if (!ptr) {
		return;
	}

	header = ((MemoryHeader*)ptr) - 1;

	g_memory_stats.free_count++;
	if (g_memory_stats.bytes_current >= (u64)header->size) {
		g_memory_stats.bytes_current -= (u64)header->size;
	} else {
		g_memory_stats.bytes_current = 0;
	}

	free(header);
}

MemoryStats memory_get_stats(void)
{
	return g_memory_stats;
}

bool memory_has_leaks(void)
{
	return g_memory_stats.bytes_current != 0;
}