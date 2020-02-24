#include <stdio.h>

#include "ftl/ftl.h"

#include "hil/hil.h"
#include "common/cmd_internal.h"

#include "fil/fil.h"

#include "util/debug.h"

#define HOST_BLOCK_SIZE 4096

#define HOST_BLOCK_CNT 16384
static int disk_size = 4096 * HOST_BLOCK_CNT; // default 64 MiB

static nand_addr_t l2p_table[HOST_BLOCK_CNT];

void ftl_init(void)
{
	printf("ftl_init\n");
	hil_set_storage_blocks(HOST_BLOCK_CNT);
}

static void ftl_process_q(void)
{
	if (!q_empty(&ftl_wait_q)) {
		struct cmd *cmd = q_get_head(&ftl_wait_q);
		dindent(3);
		dprint("[ftl_process_q]\n");
		dindent(3);
		dprint("tag %d,%c, lba:%u, cnt:%u\n", cmd->tag, cmd->type == CMD_TYPE_RD ? 'R' : 'W',
									cmd->lba, cmd->nblock);
		if (cmd->type == CMD_TYPE_RD) {
			dindent(3);
			dprint("adding req rd\n");
			struct list_head *lp = cmd->buf;
			for (int i = 0; i < cmd->nblock; i++) {
				l2p_table[cmd->lba + i] = cmd->lba + i;
				fil_add_req(cmd, NAND_TYPE_READ, list_entry(lp, struct dma_buf, list)->buf,
						l2p_table[cmd->lba + i]);
				lp = lp->next;
			}
		} else {
			dindent(3);
			dprint("adding req wr\n");
			struct list_head *lp = cmd->buf;
			for (int i = 0; i < cmd->nblock; i++) {
				l2p_table[cmd->lba + i] = cmd->lba + i;
				fil_add_req(cmd, NAND_TYPE_WRITE, list_entry(lp, struct dma_buf, list)->buf,
						l2p_table[cmd->lba + i]);
				lp = lp->next;
			}
		}
		//q_push_tail(&nand_wait_q, cmd);
		q_pop_head(&ftl_wait_q);
		dindent(3);
	}
}

void ftl_run(void)
{
	ftl_process_q();
}
