#ifndef DMA_MEMORY_H
#define DMA_MEMORY_H

#include <stddef.h>

#define DMA_MEMALLOC_ALIGNMENT			(4)	// WORD alignment
#define DMA_MEMALLOC_FW_ALIGNMENT		(4)	// FW DATA alignment
#define DMA_MEMALLOC_BUF_ALIGNMENT		(16)	// I'm not sure the DMA alignement size.
#define DMA_MEMALLOC_DMA_ALIGNMENT		(16)

void dma_memory_init(void);
void *dma_malloc(size_t size, size_t alignment);

#endif /* DMA_MEMORY_H */
