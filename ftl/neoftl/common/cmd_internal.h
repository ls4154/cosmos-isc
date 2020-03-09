#ifndef CMD_INTERNAL_H
#define CMD_INTERNAL_H

#define CMD_TYPE_RD 0
#define CMD_TYPE_WR 1

#include <util/list.h>

struct cmd
{
    int tag;
    int type;
    unsigned int lba;
    unsigned int nblock;
    unsigned int ndone;
    struct list_head *buf;
};

struct cmd_queue
{
    volatile int head;
    volatile int tail;
    int size;
    struct cmd **arr;
};

extern struct cmd_queue init_q;
extern struct cmd_queue ftl_wait_q;
extern struct cmd_queue nand_wait_q;
extern struct cmd_queue nand_done_q;
extern struct cmd_queue write_dma_wait_q;
extern struct cmd_queue read_dma_wait_q;
extern struct cmd_queue write_dma_issue_q;
extern struct cmd_queue read_dma_issue_q;
extern struct cmd_queue dma_done_q;
extern struct cmd_queue done_q;

typedef struct _nand_addr
{
    unsigned int ch: 3;
    unsigned int way: 3;
    unsigned int block: 14;
    unsigned int page: 8;
    unsigned int valid: 1;
} nand_addr_t;

#define NAND_TYPE_READ 0
#define NAND_TYPE_PROGRAM 1
#define NAND_TYPE_ERASE 2

struct nand_req
{
    int type;
    nand_addr_t addr;
    struct cmd *cmd;
    struct list_head list;
    void *buf_main;
    void *buf_spare;
};

struct dma_buf
{
    int id; // TODO debug use only, delete later
    nand_addr_t addr;
    void *buf_main;
    void *buf_spare;
    struct list_head list;
};

static inline int q_empty(struct cmd_queue *q)
{
    return q->head == q->tail;
}
static inline int q_full(struct cmd_queue *q)
{
    return q->head == (q->tail + 1) % q->size;
}
static inline struct cmd *q_get_head(struct cmd_queue *q)
{
    return q->arr[q->head];
}
static inline void q_push_tail(struct cmd_queue *q, struct cmd *cmd)
{
    q->arr[q->tail] = cmd;
    __sync_synchronize();
    q->tail = (q->tail + 1) % q->size;
}
static inline void q_pop_head(struct cmd_queue *q)
{
    __sync_synchronize();
    q->head = (q->head + 1) % q->size;
}

struct cmd *new_cmd(void);
int out_of_cmd(void);
void del_cmd(struct cmd *cmd);

struct nand_req *new_req(void);
void del_req(struct nand_req *req);

struct list_head *dmabuf_get(int nblock);
void dmabuf_put(struct list_head *buf_list, int nblock);


void cmd_internal_init(void);

#endif
