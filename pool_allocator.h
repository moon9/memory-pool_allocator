#pragma once

#include <stdint.h>

// #define _DEBUG
// to turn on additional checks and statistics.

#define PA_DEFAULT_CACHE_LINE_SIZE 128 // bytes

typedef void * pa_t;

typedef struct _pa_description {
	uint32_t block_size; // Will align to cache line size.
	uint32_t blocks_in_chunk;
	uint32_t cache_line_size;
	void *chunk; // Call pa_get_chunk_size to allocate enought memory.
} pa_description_t;

typedef struct _pa_statistics {
	uint64_t total_allocated_count;
	uint32_t alloc_worst_cmpxchg_count;
	uint32_t free_worst_cmpxchg_count;
} pa_statistics_t;

uint64_t pa_get_chunk_size(uint32_t bsize, uint32_t bcount, uint32_t cline_size);

pa_t pa_create_allocator(const pa_description_t *desc);
void pa_release_allocator(pa_t pa);

void * pa_alloc(pa_t pa);
void pa_free(pa_t pa, void *block);

// Non-thread safe!
// Require define _DEBUG
void pa_get_statistics(pa_t pa, pa_statistics_t *stats);
