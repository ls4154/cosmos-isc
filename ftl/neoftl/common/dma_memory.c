#include <stdlib.h>

#include "dma_memory.h"

#include "util/util.h"
#include "util/debug.h"
#include "common/cosmos_plus_memory_map.h"

static unsigned long mem_base;
static unsigned long mem_size;
static unsigned long mem_top;

extern void *base_DMA;

void dma_memory_init(void)
{
    mem_base = (unsigned long)base_DMA;
    mem_top = mem_base;
    mem_size = DMA_MEMORY_BUFFER_SIZE;

    printf("dma memory init\n");
    printf("    base %lx\n", mem_base);
    printf("    end %lx\n", mem_base + mem_size);
}

void *dma_malloc(size_t size, size_t alignment)
{
    unsigned long ptr;

    if (alignment == 0)
    {
        alignment = DMA_MEMALLOC_BUF_ALIGNMENT;
    }

    ptr = ROUND_UP(mem_top, alignment);

    ASSERT(ptr + size <= mem_base + mem_size);

    mem_top = ptr + size;

    printf("dma malloc: %lu allocated\n", mem_top - mem_base);

    return (void *)ptr;
}
