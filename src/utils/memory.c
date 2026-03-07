#include "utils.h"

void *br_alloc(size_t size) {
	(void)size;
	return NULL;
}

void *br_calloc(size_t count, size_t size) {
	(void)count;
	(void)size;
	return NULL;
}

void *br_realloc(void *ptr, size_t size) {
	(void)ptr;
	(void)size;
	return NULL;
}

void br_free(void *ptr) {
	(void)ptr;
}
