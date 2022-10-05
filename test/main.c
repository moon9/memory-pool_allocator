#include "../pool_allocator.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define ARR_COUNT 4

static const uint32_t BLOCK_COUNT[ARR_COUNT] = {
	256, 4096, 65536, 131072
};

static const uint32_t BLOCK_SIZE[ARR_COUNT] = {
	256, 1024, 4096, 16384
};

static struct timespec timesp[2];

static void get_thread_time(struct timespec *timesp)
{
	clock_gettime(CLOCK_MONOTONIC, timesp);
}

static uint64_t get_ns(struct timespec timesp[2])
{
	return (timesp[1].tv_sec - timesp[0].tv_sec) * (long)1e9 + abs(timesp[1].tv_nsec - timesp[0].tv_nsec);
}

static void pa_allocator_test(uint32_t size, uint32_t count)
{
	pa_description_t desc = {
		.block_size = size,
		.blocks_in_chunk = count,
		.cache_line_size = PA_DEFAULT_CACHE_LINE_SIZE,
	};

	void **block = malloc(count * sizeof(void *));

	uint64_t pa_size = pa_get_chunk_size(size, count, 16);
	desc.chunk = malloc(pa_size);

	pa_t allocator = pa_create_allocator(&desc);

	get_thread_time(&timesp[0]);
	for(uint32_t i = 0; i < count; i++)
		block[i] = pa_alloc(allocator);

	for(uint32_t i = 0; i < count; i++)
		pa_free(allocator, block[i]);
	get_thread_time(&timesp[1]);

	pa_release_allocator(allocator);

	free(desc.chunk);
	free(block);
}

static void malloc_test(uint32_t size, uint32_t count)
{
	void **block = malloc(count * sizeof(void *));

	get_thread_time(&timesp[0]);
	for(uint32_t i = 0; i < count; i++)
		block[i] = malloc(size);

	for(uint32_t i = 0; i < count; i++)
		free(block[i]);
	get_thread_time(&timesp[1]);

	free(block);
}

static void pa_allocator_value_test(uint32_t size, uint32_t count)
{
	const uint8_t TEST_VALUE = 0xCF;

	pa_description_t desc = {
		.block_size = size,
		.blocks_in_chunk = count,
		.cache_line_size = PA_DEFAULT_CACHE_LINE_SIZE,
	};

	void **block = malloc(count * sizeof(void *));

	uint64_t pa_size = pa_get_chunk_size(size, count, PA_DEFAULT_CACHE_LINE_SIZE);
	desc.chunk = malloc(pa_size);

	pa_t allocator = pa_create_allocator(&desc);

	for(uint32_t i = 0; i < count; i++) {
		block[i] = pa_alloc(allocator);
		*((uint8_t *)block[i]) = TEST_VALUE;
	}

	for(uint32_t i = 0; i < count; i++) {
		uint8_t *test_value = block[i];
		if (*test_value != TEST_VALUE) {
			printf("TEST FAILED\n");
			exit(1);
		}
		pa_free(allocator, block[i]);
	}

	pa_release_allocator(allocator);

	free(desc.chunk);
	free(block);
}

int main(void)
{
	for (uint32_t i = 0; i < ARR_COUNT; i++)
		for (uint32_t j = 0; j < ARR_COUNT; j++) {
		pa_allocator_test(BLOCK_SIZE[j], BLOCK_COUNT[i]);
		printf("Time pa b - %u c - %u\t - %lu\n", BLOCK_SIZE[j], BLOCK_COUNT[i], get_ns(timesp));

		malloc_test(BLOCK_SIZE[j], BLOCK_COUNT[i]);
		printf("Time malloc b - %u c - %u\t - %lu\n", BLOCK_SIZE[j], BLOCK_COUNT[i], get_ns(timesp));
	}

	pa_description_t desc = {
		.block_size = 76,
		.blocks_in_chunk = 8,
		.cache_line_size = PA_DEFAULT_CACHE_LINE_SIZE,
	};

	void **block = malloc(8 * sizeof(void *));

	uint64_t pa_size = pa_get_chunk_size(76, 8, PA_DEFAULT_CACHE_LINE_SIZE);
	desc.chunk = malloc(pa_size);

	printf("pa_size = %lu\n", pa_size);

	pa_t allocator = pa_create_allocator(&desc);

	printf("allocator addr\t - %p\n", desc.chunk);

	for(uint32_t i = 0; i < 8; i++) {
		block[i] = pa_alloc(allocator);
		printf("block addr\t - %p\n", block[i]);
	}

	for(uint32_t i = 0; i < 8; i++)
		pa_free(allocator, block[i]);

	pa_release_allocator(allocator);

	free(desc.chunk);
	free(block);

	pa_allocator_value_test(256, 4096);

	printf("test success\n");

	return 0;
}
