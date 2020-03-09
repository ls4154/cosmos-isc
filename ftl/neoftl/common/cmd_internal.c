#include <stdio.h>
#include <stdlib.h>

#include "cmd_internal.h"
#include "common/dma_memory.h"
#include "util/list.h"

#include "util/debug.h"

#define DEFAULT_QSIZE 128

#define BLOCK_SIZE 4096
#define NAND_PAGE_SIZE 16384
#define NAND_SPARE_SIZE 256

struct cmd_queue init_q;
struct cmd_queue ftl_wait_q;
struct cmd_queue nand_wait_q;
//struct cmd_queue nand_issue_q;
struct cmd_queue write_dma_wait_q;
struct cmd_queue read_dma_wait_q;
struct cmd_queue write_dma_issue_q;
struct cmd_queue read_dma_issue_q;
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
    {
        return ret;
    }
    ret = q_get_head(&cmd_pool_q);
    q_pop_head(&cmd_pool_q);
    return ret;
}

int out_of_cmd(void)
{
    return q_empty(&cmd_pool_q);
}

void del_cmd(struct cmd *cmd)
{
    q_push_tail(&cmd_pool_q, cmd);
}



static struct cmd_queue req_pool_q;

#define REQ_POOL_SIZE (CMD_POOL_SIZE * 32)
static struct nand_req *req_mem;

static void req_pool_init(void)
{
    init_cmd_queue(&req_pool_q, REQ_POOL_SIZE + 1);

    req_mem = malloc(REQ_POOL_SIZE * sizeof(struct nand_req));
    if (!req_mem)
    {
        perror("malloc req");
        exit(1);
    }

    for(int i = 0; i < REQ_POOL_SIZE; i++)
    {
        struct nand_req *r = req_mem + i;
        q_push_tail(&req_pool_q, (struct cmd *)r);
    }
}

struct nand_req *new_req(void)
{
    struct nand_req *ret = NULL;
    if (q_empty(&req_pool_q))
    {
        return ret;
    }
    ret = (struct nand_req *)q_get_head(&req_pool_q);
    q_pop_head(&req_pool_q);
    return ret;
}

void del_req(struct nand_req *req)
{
    q_push_tail(&req_pool_q, (struct cmd *)req);
}


#define DMA_PAGE_MAX (256 * 32)

static struct dma_buf *dma_meta;
static void *dma_mem_main;
static void *dma_mem_spare;

static int dma_buf_cnt;
static LIST_HEAD(dma_buf_list);

static void dmabuf_init(void)
{
    printf("dmabuf init\n");
    dma_mem_main = dma_malloc(NAND_PAGE_SIZE * DMA_PAGE_MAX, 0);
    dma_mem_spare = dma_malloc(NAND_SPARE_SIZE * DMA_PAGE_MAX, 0);
    dma_meta = malloc(sizeof(struct dma_buf) * DMA_PAGE_MAX);
    for (int i = 0; i < DMA_PAGE_MAX; i++)
    {
        struct dma_buf *db = dma_meta + i;
        db->id = i;
        db->buf_main = dma_mem_main + NAND_PAGE_SIZE * i;
        db->buf_spare = dma_mem_spare + NAND_SPARE_SIZE * i;
        list_add_tail(&db->list, &dma_buf_list);
    }
    /* struct dma_buf *p; */
    /* list_for_each_entry(p, &dma_buf_list, list) { */
    /*         printf("%d:%p\n", p->id, p->buf_main); */
    /* } */
    dma_buf_cnt = DMA_PAGE_MAX;
}

struct list_head *dmabuf_get(int nblock)
{
    if (dma_buf_cnt < nblock)
        return NULL;

    struct list_head *ret = dma_buf_list.next;
    if (dma_buf_cnt == nblock)
    {
        dma_buf_list.prev->next = dma_buf_list.next;
        dma_buf_list.next->prev = dma_buf_list.prev;
        dma_buf_cnt = 0;
        INIT_LIST_HEAD(&dma_buf_list);
        dprint("----------------- "
                "last! dmabuf_get first %d, last %d, left %d"
                "------------\n",
                list_entry(ret, struct dma_buf, list)->id,
                list_entry(ret->prev, struct dma_buf, list)->id,
                dma_buf_cnt
              );
        return ret;
    }
    struct list_head *p = &dma_buf_list;
    for (int i = 0; i < nblock; i++)
    {
        p = p->next;
    }

    p->next->prev = &dma_buf_list;
    dma_buf_list.next = p->next;

    p->next = ret;
    ret->prev = p;

    dma_buf_cnt -= nblock;

    dprint("----------------- "
            "dmabuf_get first %d, last %d, left %d"
            "------------\n",
            list_entry(ret, struct dma_buf, list)->id,
            list_entry(ret->prev, struct dma_buf, list)->id,
            dma_buf_cnt
          );
    /* printf("----------------- " */
    /*         "dmabuf_get %d, first %p, left %d" */
    /*         "------------\n", */
    /*         nblock, dma_buf_list.next, dma_buf_cnt); */

    /* struct list_head *p2 = ret; */
    /* for (;; ) { */
    /*         struct dma_buf *db = list_entry(p2, struct dma_buf, list); */
    /*         printf("----------------- " */
    /*                 "%d:%p" */
    /*                 "------------\n", */
    /*                 db->id, db->buf_main); */
    /*         p2 = p2->next; */
    /*         if (p2 == ret) */
    /*                 break; */
    /* } */

    return ret;
}

void dmabuf_put(struct list_head *buf_list, int nblock)
{
    /* printf("return\n"); */
    /* printf("%d:%p\n", */
    /*         list_entry(buf_list, struct dma_buf, list)->id, */
    /*         list_entry(buf_list, struct dma_buf, list)->buf_main */
    /* ); */
    /* printf("%d:%p\n", */
    /*         list_entry(buf_list->prev, struct dma_buf, list)->id, */
    /*         list_entry(buf_list->prev, struct dma_buf, list)->buf_main */
    /* ); */
    buf_list->prev->next = dma_buf_list.next;
    dma_buf_list.next->prev = buf_list->prev;

    buf_list->prev = &dma_buf_list;
    dma_buf_list.next = buf_list;

    dma_buf_cnt += nblock;

    /* struct list_head *p2 = dma_buf_list.next; */
    /* for (int i = 0; i < 15; i++) { */
    /*         struct dma_buf *db = list_entry(p2, struct dma_buf, list); */
    /*         printf("%d:%p\n", db->id, db->buf_main); */
    /*         p2 = p2->next; */
    /* } */
}

void cmd_internal_init(void)
{
    /* queue init */
    init_cmd_queue(&init_q, DEFAULT_QSIZE);
    init_cmd_queue(&ftl_wait_q, DEFAULT_QSIZE);
    init_cmd_queue(&nand_wait_q, DEFAULT_QSIZE);
    //init_cmd_queue(&nand_issue_q, DEFAULT_QSIZE);
    init_cmd_queue(&write_dma_wait_q, DEFAULT_QSIZE);
    init_cmd_queue(&read_dma_wait_q, DEFAULT_QSIZE);
    init_cmd_queue(&write_dma_issue_q, DEFAULT_QSIZE);
    init_cmd_queue(&read_dma_issue_q, DEFAULT_QSIZE);
    init_cmd_queue(&done_q, DEFAULT_QSIZE);

    /* cmd struct pool init */
    init_cmd_queue(&cmd_pool_q, CMD_POOL_SIZE + 1);

    cmd_mem = malloc(CMD_POOL_SIZE * sizeof(struct cmd));
    if (!cmd_mem)
    {
        perror("malloc cmd");
        exit(1);
    }

    for (int i = 0; i < CMD_POOL_SIZE; i++)
    {
        struct cmd *c = cmd_mem + i;
        q_push_tail(&cmd_pool_q, c);
    }

    dmabuf_init();

    req_pool_init();
}
