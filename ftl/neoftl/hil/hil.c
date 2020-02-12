#include "hil.h"
#include "nvme_main.h"
#include "host_lld.h"

#include "util/debug.h"
#include "util/list.h"

#include "common/cmd_internal.h"
#include "common/cosmos_types.h"
#include "common/cosmos_plus_system.h"

#include "util/debug.h"

unsigned int storageCapacity_L;

void hil_init(void)
{
	printf("hil_init\n");
	DEBUG_ASSERT(LOGICAL_PAGE_SIZE == HOST_BLOCK_SIZE); // if this is not same, HIL must convert block number to LPN
}

void hil_process_initq(void);
void hil_process_dmaq(void);
void hil_process_doneq(void);

void hil_run(void)
{
	nvme_run();
	hil_process_doneq();
	hil_process_dmaq();
	hil_process_initq();
}

void hil_set_storage_blocks(unsigned int nblock)
{
	printf("set storage block: %u blocks\n", nblock);
	storageCapacity_L = nblock;
	ASSERT(storageCapacity_L > 0);
}

unsigned int hil_get_storage_blocks(void)
{
	return storageCapacity_L;
}

void hil_read_block(unsigned int cmd_tag, unsigned int start_lba, unsigned int lba_count)
{
	struct cmd *cmd;
	dprint("[hil read block]\n");
	do {
		cmd = new_cmd();
	} while (cmd == NULL);
	dprint("cmd allocated\n");

	cmd->tag = cmd_tag;
	cmd->type = CMD_TYPE_RD;
	cmd->lba = start_lba;
	cmd->nblock = lba_count;
	cmd->buf = NULL;

	dprint("tag: %d, lba: %u, cnt: %u\n", cmd_tag, start_lba, lba_count);

	while (q_full(&init_q));
	q_push_tail(&init_q, cmd);
}

void hil_write_block(unsigned int cmd_tag, unsigned int start_lba, unsigned int lba_count)
{
	struct cmd *cmd;
	dprint("[hil write block]\n");
	do {
		cmd = new_cmd();
	} while (cmd == NULL);
	dprint("cmd allocated\n");

	cmd->tag = cmd_tag;
	cmd->type = CMD_TYPE_WR;
	cmd->lba = start_lba;
	cmd->nblock = lba_count;
	cmd->buf = NULL;

	dprint("tag: %u, lba: %u, cnt: %u\n", cmd_tag, start_lba, lba_count);

	while (q_full(&init_q));
	q_push_tail(&init_q, cmd);
}

void hil_process_initq(void)
{
	while (!q_empty(&init_q)) {
		struct cmd *cmd = q_get_head(&init_q);
		dprint("[hil_process_initq]\n");
		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);
		cmd->buf = dmabuf_get(cmd->nblock);
		if (cmd->buf == NULL)
			return;
		if (cmd->type == CMD_TYPE_RD) {
			while (q_full(&ftl_wait_q));
			q_push_tail(&ftl_wait_q, cmd);
		} else {
			while (q_full(&write_dma_wait_q));
			q_push_tail(&write_dma_wait_q, cmd);
		}
		q_pop_head(&init_q);
	}
}

void hil_process_dmaq(void)
{
	// TODO async dma check
	if (!q_empty(&write_dma_wait_q)) {
		dprint("[hil_process_dmaq] write dma\n");
		struct cmd *cmd = q_get_head(&write_dma_wait_q);
		for (int i = 0; i < cmd->nblock; i++)
			set_auto_rx_dma(cmd->tag, i, (unsigned int)(cmd->buf + i * 4096),
					NVME_COMMAND_AUTO_COMPLETION_ON);
		check_auto_rx_dma_done();
		dprint("dma done\n");
		while (q_full(&ftl_wait_q));
		q_push_tail(&ftl_wait_q, cmd);
		q_pop_head(&write_dma_wait_q);
	}
	if (!q_empty(&read_dma_wait_q)) {
		dprint("[hil_process_dmaq] read dma\n");
		struct cmd *cmd = q_get_head(&read_dma_wait_q);
		for (int i = 0; i < cmd->nblock; i++)
			set_auto_tx_dma(cmd->tag, i, (unsigned int)(cmd->buf + i * 4096),
					NVME_COMMAND_AUTO_COMPLETION_ON);
		check_auto_tx_dma_done();
		dprint("dma done\n");

		dmabuf_put(cmd->buf);
		del_cmd(cmd);
		q_pop_head(&read_dma_wait_q);
	}
}

void hil_process_doneq(void)
{
	while (!q_empty(&done_q)) {
		dprint("[hil_process_doneq]\n");
		struct cmd *cmd = q_get_head(&done_q);
		q_pop_head(&done_q);
		dmabuf_put(cmd->buf);
		del_cmd(cmd);
	}
}
