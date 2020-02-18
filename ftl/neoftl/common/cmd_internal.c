#include <stdio.h>
#include <stdlib.h>

#include "cmd_internal.h"
#include "common/dma_memory.h"
#include "util/list.h"

#include "util/debug.h"

#define DEFAULT_QSIZE 128

#define BLOCK_SIZE 4096

struct cmd_queue init_q;
struct cmd_queue ftl_wait_q;
struct cmd_queue nand_wait_q;
struct cmd_queue nand_issue_q;
struct cmd_queue write_dma_wait_q;
struct cmd_queue read_dma_wait_q;
struct cmd_queue dma_issue_q;
struct cmd_queue done_q;

static struct cmd_queue cmd_pool_q;

#define CMD_POOL_SIZE 512
static struct cmd *cmd_mem;

static void init_cmd_queue(struct cmd_queue *q, int size)
{
	q->head = 0;
	q->tail = 0;
	q->size = size;
	q->arr = malloc(size * sizeof(struct cmd *));
}


struct cmd *new_cmd(void)
{
	struct cmd *ret = NULL;
	if (q_empty(&cmd_pool_q))
		return ret;
	ret = q_get_head(&cmd_pool_q);
	q_pop_head(&cmd_pool_q);
	return ret;
}

void del_cmd(struct cmd *cmd)
{
	q_push_tail(&cmd_pool_q, cmd);
}

/* static void *dma_mem; */
/* static int dma_head; */
/* static int dma_tail; */
/* static int dma_skip; */

#define PAGE_MAX_1 256
#define PAGE_MAX_256 128

static struct dma_buf *dma_meta_1;
static struct dma_buf *dma_meta_256;
static void *dma_mem_1;
static void *dma_mem_256;

LIST_HEAD(dma_list_1);
LIST_HEAD(dma_list_256);

struct dma_buf {
	int size;
	void *buf;
	struct list_head list;
};

void *dmabuf_get(int nblock)
{
	if (nblock == 1 && !list_empty(&dma_list_1)) {
		struct dma_buf *db = list_first_entry(&dma_list_1, struct dma_buf, list);
		list_del(&db->list);
		return db->buf;
	}
	if (!list_empty(&dma_list_256)) {
		struct dma_buf *db = list_first_entry(&dma_list_256, struct dma_buf, list);
		list_del(&db->list);
		return db->buf;
	}
	printf("dmabuf_get: outof dmabuf\n");
	return NULL;
}
void dmabuf_put(void *ptr)
{
	struct dma_buf *db;
	if (ptr >= dma_mem_1 && ptr < dma_mem_1 + BLOCK_SIZE * PAGE_MAX_1) {
		db = dma_meta_1 + ((ptr - dma_mem_1) / BLOCK_SIZE);
		ASSERT(db->size == 1);
		list_add_tail(&db->list, &dma_list_1);
	} else {
		db = dma_meta_256 + ((ptr - dma_mem_256) / (256 * BLOCK_SIZE));
		list_add_tail(&db->list, &dma_list_256);
		ASSERT(db->size == 256);
	}
}

void cmd_internal_init(void)
{
	init_cmd_queue(&init_q, DEFAULT_QSIZE);
	init_cmd_queue(&ftl_wait_q, DEFAULT_QSIZE);
	init_cmd_queue(&nand_wait_q, DEFAULT_QSIZE);
	init_cmd_queue(&nand_issue_q, DEFAULT_QSIZE);
	init_cmd_queue(&write_dma_wait_q, DEFAULT_QSIZE);
	init_cmd_queue(&read_dma_wait_q, DEFAULT_QSIZE);
	init_cmd_queue(&dma_issue_q, DEFAULT_QSIZE);
	init_cmd_queue(&done_q, DEFAULT_QSIZE);

	init_cmd_queue(&cmd_pool_q, CMD_POOL_SIZE + 1);

	cmd_mem = malloc(CMD_POOL_SIZE * sizeof(struct cmd));
	if (!cmd_mem) {
		perror("malloc");
		exit(1);
	}

	for (int i = 0; i < CMD_POOL_SIZE; i++) {
		struct cmd *c = cmd_mem + i;
		q_push_tail(&cmd_pool_q, c);
	}

	dma_mem_1 = dma_malloc(BLOCK_SIZE * PAGE_MAX_1, 0);
	dma_mem_256 = dma_malloc(BLOCK_SIZE * 256 * PAGE_MAX_256, 0);
	dma_meta_1 = malloc(sizeof(struct dma_buf) * PAGE_MAX_1);
	dma_meta_256 = malloc(sizeof(struct dma_buf) * PAGE_MAX_256);
	printf("cmd_internal_init: dma_mem_1   %p to %p\n", dma_mem_1, dma_mem_1 + BLOCK_SIZE * PAGE_MAX_1);
	printf("cmd_internal_init: dma_mem_256 %p to %p\n", dma_mem_256, dma_mem_256 + BLOCK_SIZE * 256 * PAGE_MAX_256);
	for (int i = 0; i < PAGE_MAX_1; i++) {
		struct dma_buf *db = dma_meta_1 + i;
		db->size = 1;
		db->buf = dma_mem_1 + BLOCK_SIZE * i;
		list_add_tail(&db->list, &dma_list_1);
	}
	for (int i = 0; i < PAGE_MAX_256; i++) {
		struct dma_buf *db = dma_meta_256 + i;
		db->size = 256;
		db->buf = dma_mem_256 + 256 * BLOCK_SIZE * i;
		list_add_tail(&db->list, &dma_list_256);
	}
}
