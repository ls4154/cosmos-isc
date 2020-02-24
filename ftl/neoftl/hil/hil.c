#include <signal.h>

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

int start_debug = 0;
static int status;

static int tmp_status = 0;
static struct cmd tmp_cmd;

static unsigned int dma_tx_head = 0;
static unsigned int dma_tx_tail = 0;
static unsigned int dma_rx_head = 0;
static unsigned int dma_rx_tail = 0;

static inline int dma_tx_slots(void)
{
	return (dma_tx_tail - dma_tx_head + 256) % 256;
}
static inline int dma_rx_slots(void)
{
	return 0;
}

static void sig_debug(int sig)
{
	printf("== SIGUSR2 debug status %d == ", status);
	start_debug = !start_debug;
}

void hil_init(void)
{
	printf("hil_init\n");
	DEBUG_ASSERT(LOGICAL_PAGE_SIZE == HOST_BLOCK_SIZE); // if this is not same, HIL must convert block number to LPN

	signal(SIGUSR2, sig_debug);
}

void hil_process_init(void);
void hil_process_dmawait(void);
void hil_process_dmadone(void);
void hil_process_doneq(void);
void hil_proces_tmp(void);

void hil_run(void)
{
	if (!tmp_status)
		nvme_run();
	else
		hil_proces_tmp();
	status++;
	hil_process_doneq();
	status++;
	hil_process_dmawait();
	status++;
	hil_process_init();
	status = 0;

	if (start_debug) {
		puts("======================================");
		start_debug = 0;
	}
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
	cmd = new_cmd();
	if (cmd == NULL || q_full(&init_q)) {
		tmp_status = 1;

		tmp_cmd.tag = cmd_tag;
		tmp_cmd.type = CMD_TYPE_RD;
		tmp_cmd.lba = start_lba;
		tmp_cmd.nblock = lba_count;
		tmp_cmd.buf = NULL;

		dprint("new cmd NULL exit\n");
		return;
	}

	cmd->tag = cmd_tag;
	cmd->type = CMD_TYPE_RD;
	cmd->lba = start_lba;
	cmd->nblock = lba_count;
	cmd->ndone = 0;
	cmd->buf = NULL;

	dprint("tag: %d, lba: %u, cnt: %u\n", cmd_tag, start_lba, lba_count);

	ASSERT(!q_full(&init_q));

	q_push_tail(&init_q, cmd);
}

void hil_write_block(unsigned int cmd_tag, unsigned int start_lba, unsigned int lba_count)
{
	struct cmd *cmd;
	dprint("[hil write block]\n");
	cmd = new_cmd();
	if (cmd == NULL || q_full(&init_q)) {
		tmp_status = 1;

		tmp_cmd.tag = cmd_tag;
		tmp_cmd.type = CMD_TYPE_WR;
		tmp_cmd.lba = start_lba;
		tmp_cmd.nblock = lba_count;
		tmp_cmd.buf = NULL;

		dprint("new cmd NULL exit\n");
		return;
	}

	cmd->tag = cmd_tag;
	cmd->type = CMD_TYPE_WR;
	cmd->lba = start_lba;
	cmd->nblock = lba_count;
	cmd->ndone = 0;
	cmd->buf = NULL;

	dprint("tag: %u, lba: %u, cnt: %u\n", cmd_tag, start_lba, lba_count);

	ASSERT(!q_full(&init_q));

	q_push_tail(&init_q, cmd);
}

void hil_process_init(void)
{
	while (!q_empty(&init_q)) {
		struct cmd *cmd = q_get_head(&init_q);
		dprint("[hil_process_init]\n");
		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);
		if (cmd->buf == NULL)
			cmd->buf = dmabuf_get(cmd->nblock);
		if (cmd->buf == NULL)
			return;
		if (cmd->type == CMD_TYPE_RD) {
			if (q_full(&ftl_wait_q)) {
				dprint("hil process initq rd full\n");
				return;
			}
			q_push_tail(&ftl_wait_q, cmd);
		} else {
			if (q_full(&write_dma_wait_q)) {
				dprint("hil process initq wr full\n");
				return;
			}
			q_push_tail(&write_dma_wait_q, cmd);
		}
		dprint("pop from init_q\n");
		q_pop_head(&init_q);
	}
}

void hil_process_dmawait(void)
{
	// TODO async dma check
	if (!q_empty(&write_dma_wait_q)) {
		dprint("[hil_process_dmawait] write dma\n");
		struct cmd *cmd = q_get_head(&write_dma_wait_q);
		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);
		struct list_head *lp = cmd->buf;
		for (int i = 0; i < cmd->nblock; i++) {
			set_auto_rx_dma(cmd->tag, i, (unsigned int)list_entry(lp, struct dma_buf, list)->buf,
					NVME_COMMAND_AUTO_COMPLETION_ON);
			lp = lp->next;
			dma_rx_tail++;
		}
		check_auto_rx_dma_done();
		dprint("dma done\n");
		printf("[%d]\n", ((char *)list_entry(cmd->buf, struct dma_buf, list)->buf)[0]);
		while (q_full(&ftl_wait_q)) {
			dprint("ftl wait q full\n");
		}
		q_push_tail(&ftl_wait_q, cmd);
		q_pop_head(&write_dma_wait_q);
	}
	if (!q_empty(&read_dma_wait_q)) {
		dprint("[hil_process_dmawait] read dma\n");
		struct cmd *cmd = q_get_head(&read_dma_wait_q);
		printf("[%d]\n", ((char *)list_entry(cmd->buf, struct dma_buf, list)->buf)[0]);
		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);
		struct list_head *lp = cmd->buf;
		for (int i = 0; i < cmd->nblock; i++) {
			set_auto_tx_dma(cmd->tag, i, (unsigned int)list_entry(lp, struct dma_buf, list)->buf,
					NVME_COMMAND_AUTO_COMPLETION_ON);
			lp = lp->next;
			dma_tx_tail++;
		}
		check_auto_tx_dma_done();
		dprint("dma done\n");

		q_pop_head(&read_dma_wait_q);
		dmabuf_put(cmd->buf, cmd->nblock);
		dprint("delet cmd\n");
		del_cmd(cmd);
	}
}

void hil_process_dmadone(void)
{
}

void hil_process_doneq(void)
{
	while (!q_empty(&done_q)) {
		dprint("[hil_process_doneq]\n");
		struct cmd *cmd = q_get_head(&done_q);
		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);
		q_pop_head(&done_q);
		dmabuf_put(cmd->buf, cmd->nblock);
		dprint("delete cmd\n");
		del_cmd(cmd);
	}
}

void hil_proces_tmp(void)
{
	struct cmd *cmd;
	dprint("[hil read block]\n");
	cmd = new_cmd();
	if (cmd == NULL || q_full(&init_q)) {
		dprint("new cmd NULL exit\n");
		return;
	}

	cmd->tag = tmp_cmd.tag;
	cmd->type = tmp_cmd.type;
	cmd->lba = tmp_cmd.lba;
	cmd->nblock = tmp_cmd.nblock;
	cmd->ndone = 0;
	cmd->buf = tmp_cmd.buf;

	dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);

	ASSERT(!q_full(&init_q));
	q_push_tail(&init_q, cmd);

	tmp_status = 0;
}
