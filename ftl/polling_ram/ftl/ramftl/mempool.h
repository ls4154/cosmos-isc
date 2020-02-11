#ifndef MEMPOOL_H
#define MEMPOOL_H

void mempool_init(void);
void *mempool_dmabuf_get(int nblocks);
void mempool_put(int nblocks);

#endif
