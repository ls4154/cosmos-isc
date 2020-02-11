#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fil/fil.h"

#include "common/cmd_internal.h"

static void *disk;

void fil_init(void)
{
	int disk_size = (1 << 25); // default 32 MiB
	disk = malloc(disk_size);
	memset(disk, 0xFF, disk_size);
	if (disk == NULL) {
		perror("malloc disk");
		exit(1);
	}
	printf("fil init: disk size %d\n", disk_size);
	printf("start %p, end %p\n", disk, disk + disk_size);
}


static void fil_process_nandq(void)
{
	while (!q_empty(&nand_wait_q)) {
		struct cmd *cmd = q_get_head(&nand_wait_q);
		printf("===================================");
		printf("fil_process_nandq: tag %d,%c, lba:%u, cnt:%u\n", cmd->tag, cmd->type == CMD_TYPE_RD ? 'R' : 'W',
							cmd->lba, cmd->nblock);
		printf("===================================");
		printf("                  disk start %p, end %p\n",
				disk + 4096 * cmd->lba, disk + 4096 * cmd->lba + 4096 * cmd->nblock);
		printf("                  buf start %p, end %p\n",
				cmd->buf, cmd->buf + 4096 * cmd->nblock);
		cmd->ndone = cmd->nblock;
		printf("===================================copy start\n");
		if (cmd->type == CMD_TYPE_RD) {
			memcpy(cmd->buf, disk + 4096 * cmd->lba, 4096 * cmd->nblock);
			printf("===================================copy done\n");
			while (q_full(&read_dma_wait_q));
			q_push_tail(&read_dma_wait_q, cmd);
		} else {
			memcpy(disk + 4096 * cmd->lba, cmd->buf, 4096 * cmd->nblock);
			printf("===================================copy done\n");
			while (q_full(&done_q));
			q_push_tail(&done_q, cmd);
		}
		printf("===================================q_pop\n");
		q_pop_head(&nand_wait_q);
	}
}

void fil_run(void)
{
	fil_process_nandq();
}

