#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fil/fil.h"

#include "hil/hil.h"

#include "common/cmd_internal.h"

#include "util/debug.h"

#define BLOCK_SIZE 4096

static void *disk;

void fil_init(void)
{
	unsigned int disk_size = hil_get_storage_blocks() * BLOCK_SIZE;

	printf("fil init: disk size %u\n", disk_size);

	disk = malloc(disk_size);
	if (disk == NULL) {
		perror("malloc disk");
		exit(1);
	}
	memset(disk, 0xFF, disk_size);
	printf("start %p, end %p\n", disk, disk + disk_size);
}


static void fil_process_nandq(void)
{
	if (!q_empty(&nand_wait_q)) {
		struct cmd *cmd = q_get_head(&nand_wait_q);
		dindent(6);
		dprint("[fil_process_nandq]\n");
		cmd->ndone = cmd->nblock;
		dindent(6);
		dprint("tag %d,%c, lba:%u, cnt:%u\n", cmd->tag, cmd->type == CMD_TYPE_RD ? 'R' : 'W',
									cmd->lba, cmd->nblock);
		if (cmd->type == CMD_TYPE_RD) {
			memcpy(cmd->buf, disk + BLOCK_SIZE * cmd->lba, BLOCK_SIZE * cmd->nblock);
			dindent(6);
			dprint("rd copy done\n");
			while (q_full(&read_dma_wait_q)) {
				puts("fil nandq q full");
			}
			q_push_tail(&read_dma_wait_q, cmd);
		} else {
			memcpy(disk + BLOCK_SIZE * cmd->lba, cmd->buf, BLOCK_SIZE * cmd->nblock);
			dindent(6);
			dprint("wr copy done\n");
			while (q_full(&done_q)) {
				puts("fil nandq q full");
				sleep(1);
			};
			q_push_tail(&done_q, cmd);
		}
		dindent(6);
		dprint("q_pop\n");
		q_pop_head(&nand_wait_q);
	}
}

void fil_run(void)
{
	fil_process_nandq();
}

