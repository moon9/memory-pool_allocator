#include "pool_allocator.h"

#include <stddef.h>
#include <string.h>

#if defined (_DEBUG)
	#include <assert.h>
#endif

#define ALIGN_TO_CACHE_LINE(x, y) ((x + (y)) & ~(y))

typedef struct _pa_block {
	struct _pa_block *next;
} pa_block_t;

typedef struct _pa_allocator {
	pa_description_t desc;
	pa_block_t *current_block;

#if defined (_DEBUG)
	pa_statistics_t stats;
#endif

} pa_allocator_t;

static pa_allocator_t * get_allocator(pa_t pa)
{
#if defined (_DEBUG)
	assert(pa);
#endif

	return pa;
}

static pa_block_t * divide_chunk_on_blocks(const pa_description_t *desc)
{
	uint32_t allocator_size = ALIGN_TO_CACHE_LINE(sizeof(pa_allocator_t),
		desc->cache_line_size - 1);
	uint32_t bsize = ALIGN_TO_CACHE_LINE(desc->block_size,
		desc->cache_line_size - 1);
	pa_block_t *first = desc->chunk + allocator_size;
	pa_block_t *tmp = first;

	for (uint32_t i = 1; i < desc->blocks_in_chunk; i++)
		tmp = tmp->next = (void *)tmp + bsize;

	tmp->next = NULL;

	return first;
}

#if defined (_DEBUG)
	static void check_allocator_desc(const pa_description_t *desc)
	{
		assert(desc);
		assert(desc->block_size > 0);
		assert(desc->blocks_in_chunk > 0);
		assert(desc->cache_line_size > 0);
		assert(desc->chunk);
	}

	static void check_block(pa_t pa, void *block)
	{
		assert(block);

		pa_allocator_t *allocator = get_allocator(pa);

		uintptr_t low_addr = (uintptr_t)allocator->desc.chunk;
		uintptr_t high_addr = (uintptr_t)allocator->desc.chunk +
			pa_get_chunk_size(allocator->desc.block_size,
				allocator->desc.blocks_in_chunk,
				allocator->desc.cache_line_size);
		uintptr_t block_addr = (uintptr_t)block;

		assert(low_addr < block_addr && block_addr < high_addr);
	}

	static void clean_stats(pa_t pa)
	{
		pa_allocator_t *allocator = get_allocator(pa);
		memset(&allocator->stats, 0, sizeof(pa_statistics_t));
	}
#else
	#define check_allocator_desc(...)
	#define check_block(...)
	#define clean_stats(...)
#endif

uint64_t pa_get_chunk_size(uint32_t bsize, uint32_t bcount, uint32_t cline_size)
{
	uint32_t allocator_size = ALIGN_TO_CACHE_LINE(sizeof(pa_allocator_t),
		cline_size - 1);
	uint32_t align_bsize = ALIGN_TO_CACHE_LINE(bsize, cline_size - 1);

	return allocator_size + align_bsize * bcount;
}

pa_t pa_create_allocator(const pa_description_t *desc)
{
	check_allocator_desc(desc);

	pa_allocator_t *allocator = get_allocator(desc->chunk);

	memcpy(&allocator->desc, desc, sizeof(pa_description_t));
	allocator->current_block = divide_chunk_on_blocks(desc);
	clean_stats(allocator);

	return allocator;
}

void pa_release_allocator(pa_t pa)
{
	(void)pa;
}

void * pa_alloc(pa_t pa)
{
	pa_allocator_t *allocator = get_allocator(pa);
	pa_block_t *alloc_block = NULL;
	pa_block_t *next_block = NULL;

#if defined (_DEBUG)
	uint32_t worst_count = 0;
#endif

	do {
		alloc_block = allocator->current_block;
		next_block = alloc_block->next;

#if defined (_DEBUG)
		worst_count++;
#endif

	} while (!__sync_bool_compare_and_swap(&allocator->current_block,
		alloc_block, next_block));

#if defined (_DEBUG)
	assert(alloc_block);

	__sync_fetch_and_add(&allocator->stats.total_allocated_count, 1);

	do {
	} while (allocator->stats.alloc_worst_cmpxchg_count < worst_count &&
		!__sync_bool_compare_and_swap(&allocator->stats.alloc_worst_cmpxchg_count,
			allocator->stats.alloc_worst_cmpxchg_count, worst_count));
#endif

	return alloc_block;
}

void pa_free(pa_t pa, void *block)
{
	check_block(pa, block);

	pa_allocator_t *allocator = get_allocator(pa);
	pa_block_t *new_curr_block = block;
	pa_block_t *curr_block = NULL;

#if defined (_DEBUG)
	uint32_t worst_count = 0;
#endif

	do {
		new_curr_block->next = allocator->current_block;
		curr_block = allocator->current_block;

#if defined (_DEBUG)
		worst_count++;
#endif

	} while (!__sync_bool_compare_and_swap(&allocator->current_block,
		curr_block, new_curr_block));

#if defined (_DEBUG)
	do {
	} while (allocator->stats.free_worst_cmpxchg_count < worst_count &&
		!__sync_bool_compare_and_swap(&allocator->stats.free_worst_cmpxchg_count,
			allocator->stats.free_worst_cmpxchg_count, worst_count));
#endif
}

void pa_get_statistics(pa_t pa, pa_statistics_t *stats)
{
#if defined (_DEBUG)
	assert(stats);

	pa_allocator_t *allocator = get_allocator(pa);

	memcpy(stats, &allocator->stats, sizeof(pa_statistics_t));
#else
	(void)pa;
	(void)stats;
#endif
}
