#ifndef CMD_INTERNAL_H
#define CMD_INTERNAL_H

#define CMD_TYPE_RD 0
#define CMD_TYPE_WR 1

struct cmd {
	int tag;
	int type;
	unsigned int lba;
	unsigned int nblock;
	unsigned int ndone;
	struct list_head *buf;
	// v2p mapping
};

struct cmd_queue {
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
extern struct cmd_queue dma_issue_q;
extern struct cmd_queue dma_done_q;
extern struct cmd_queue done_q;

typedef struct _nand_addr {
	unsigned int ch: 3;
	unsigned int way: 3;
	unsigned int page: 8;
	unsigned int block: 14;
} nand_addr_t;

struct nand_req {
	int type;
	struct cmd_struct *cmd;
	void *buf;
};


struct dma_buf {
	void *buf;
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
void del_cmd(struct cmd *cmd);

struct dma_buf *dmabuf_get(int nblock);
void dmabuf_put(void* ptr);

void cmd_internal_init(void);

#endif
