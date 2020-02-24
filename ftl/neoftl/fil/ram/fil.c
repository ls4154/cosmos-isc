#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fil/fil.h"

#include "hil/hil.h"

#include "common/cmd_internal.h"

#include "util/debug.h"

#define HOST_BLOCK_SIZE 4096

static void *disk;

LIST_HEAD(nand0_q);

void fil_init(void)
{
	unsigned int disk_size = hil_get_storage_blocks() * HOST_BLOCK_SIZE;

	printf("fil init: disk size %u\n", disk_size);

	disk = malloc(disk_size);
	if (disk == NULL) {
		perror("malloc disk");
		exit(1);
	}
	memset(disk, 0xFF, disk_size);
	printf("start %p, end %p\n", disk, disk + disk_size);
}

void fil_add_req(struct cmd *cmd, int type, void *buf, nand_addr_t addr)
{
	dindent(6);
	dprint("[fil_add_req]\n");

	struct nand_req *req = malloc(sizeof(struct nand_req));
	req->type = type;
	req->addr = addr;
	req->cmd = cmd;
	req->buf = buf;

	list_add_tail(&req->list, &nand0_q);
	dindent(6);
	dprint("added to nand_q\n");
}

static void fil_nand0(void)
{
	while (!list_empty(&nand0_q)) {
		dindent(6);
		dprint("[fil_nand]\n");

		struct nand_req *req = list_first_entry(&nand0_q, struct nand_req, list);
		struct cmd *cmd = req->cmd;
		dindent(6);
		dprint("tag: %d lba %u cnt %u\n", cmd->tag, cmd->lba, cmd->nblock);
		dindent(6);
		dprint("buf: %p, disk %p\n", req->buf, disk + HOST_BLOCK_SIZE * req->addr);

		if (req->type == NAND_TYPE_READ) {
			memcpy(req->buf, disk + HOST_BLOCK_SIZE * req->addr, HOST_BLOCK_SIZE);

			dindent(6);
			dprint("rd copy done\n");
		} else {
			memcpy(disk + HOST_BLOCK_SIZE * req->addr, req->buf, HOST_BLOCK_SIZE);

			dindent(6);
			dprint("wr copy done\n");
		}

		list_del(&req->list);
		free(req);

		if (cmd->ndone == 0) {
			dindent(6);
			dprint("[%d] [%d]\n", ((char *)req->buf)[0], ((char *)disk + HOST_BLOCK_SIZE * req->addr)[0]);
		}

		cmd->ndone++;
		dindent(6);
		dprint("%u/%u\n", cmd->ndone, cmd->nblock);
		if (cmd->ndone < cmd->nblock)
			continue;

		dindent(6);
		if (cmd->type == CMD_TYPE_RD) {
			dprint("rd cmd done\n");
			q_push_tail(&read_dma_wait_q, cmd);
		} else {
			dprint("wr cmd done\n");
			q_push_tail(&done_q, cmd);
		}
		break;
	}
}

void fil_run(void)
{
	fil_nand0();
}

