#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

/**
 * Memory allocation with error handling
 * Pattern: Direct malloc with validation
 */
void *br_alloc(size_t size) {
	if (size == 0) {
		return NULL;
	}
	
	void *ptr = malloc(size);
	if (ptr == NULL) {
		SDL_Log("ERROR: Memory allocation failed for %zu bytes", size);
	}
	return ptr;
}

/**
 * Calloc wrapper: allocates and zeroes memory
 * Pattern: Initialized allocation for struct instances
 */
void *br_calloc(size_t count, size_t size) {
	if (count == 0 || size == 0) {
		return NULL;
	}
	
	void *ptr = calloc(count, size);
	if (ptr == NULL) {
		SDL_Log("ERROR: Memory allocation failed for %zu x %zu bytes", count, size);
	}
	return ptr;
}

/**
 * Realloc wrapper: resizes allocated memory
 * Pattern: Used for dynamic arrays that grow
 */
void *br_realloc(void *ptr, size_t size) {
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	
	void *new_ptr = realloc(ptr, size);
	if (new_ptr == NULL) {
		SDL_Log("ERROR: Memory reallocation failed for %zu bytes", size);
	}
	return new_ptr;
}

/**
 * Free wrapper: deallocates memory
 * Pattern: Always safe to call with NULL
 */
void br_free(void *ptr) {
	if (ptr != NULL) {
		free(ptr);
	}
}
