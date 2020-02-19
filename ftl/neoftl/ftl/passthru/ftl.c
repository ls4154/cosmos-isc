#include <stdio.h>

#include "ftl/ftl.h"

#include "hil/hil.h"
#include "common/cmd_internal.h"

#include "util/debug.h"

#define BLOCK_CNT 16384;
static int disk_size = 4096 * BLOCK_CNT; // default 64 MiB

static nand_addr_t l2p_table[BLOCK_CNT];

void ftl_init(void)
{
	printf("ftl_init\n");
	hil_set_storage_blocks(BLOCK_CNT);
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
		if (cmd->type == IO_TYPE_RD) {
			for (int i = 0; i < cmd->nblock; i++) {
				fil_req(cmd, , , l2p_table[cmd->lba + i]);
			}
		} else {
		}
		q_push_tail(&nand_wait_q, cmd);
		q_pop_head(&ftl_wait_q);
	}
}
void ftl_run(void)
{
	ftl_process_q();
}
