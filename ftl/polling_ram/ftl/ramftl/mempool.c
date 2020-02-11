#include <stdio.h>
#include <stdlib.h>

#include "mempool.h"
#include "common/dma_memory.h"

#define PAGE_MAX 4096
static void *mem;
static int head;
static int tail;
static int skip;

void mempool_init(void)
{
	mem = dma_malloc(4096 * PAGE_MAX, 0);
	head = 0;
	tail = 0;
	skip = -1;
}

void *mempool_dmabuf_get(int nblocks)
{
	void *ret = mem + 4096 * tail;

	if (tail + nblocks > PAGE_MAX) {
		skip = tail;
		tail = 0;
	}
	if (tail < head && tail + nblocks >= head) {
		printf("    mempool_dmabuf_get: full\n");
		exit(1);
	}

	printf("    mempool_dmabuf_get: %d %d %d\n", head, skip, tail);

	tail = tail + nblocks;
	return ret;
}

void mempool_put(int nblocks)
{
	head = head + nblocks;
	if (head == skip || head >= PAGE_MAX) {
		skip = -1;
		head = 0;
	}
	printf("    mempool_put: %d %d %d\n", head, skip, tail);
}
