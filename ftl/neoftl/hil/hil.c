#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

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

void hil_init(void)
{
	printf("hil_init\n");
	DEBUG_ASSERT(LOGICAL_PAGE_SIZE == HOST_BLOCK_SIZE); // if this is not same, HIL must convert block number to LPN
}

void hil_process_init(void);
void hil_process_dmawait(void);
void hil_process_dmadone(void);
void hil_process_doneq(void);

void hil_run(void)
{
    nvme_run();
	hil_process_doneq();
	hil_process_dmadone();
	hil_process_dmawait();
	hil_process_init();
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
	while (!q_empty(&init_q))
    {
		struct cmd *cmd = q_get_head(&init_q);

		dprint("[hil_process_init]\n");
		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);

		if (cmd->buf == NULL)
        {
			cmd->buf = dmabuf_get(cmd->nblock);
            if (cmd->buf == NULL)
            {
                return;
            }
        }

		if (cmd->type == CMD_TYPE_RD)
        {
			if (q_full(&ftl_wait_q))
            {
				printf("hil process initq rd full\n");
				return;
			}
			q_push_tail(&ftl_wait_q, cmd);
		}
        else
        {
			if (q_full(&write_dma_wait_q))
            {
				printf("hil process initq wr full\n");
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
	while (!q_empty(&write_dma_wait_q))
    {
		dprint("[hil_process_dmawait] write dma\n");

		struct cmd *cmd = q_get_head(&write_dma_wait_q);

		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);

		if (q_full(&write_dma_issue_q))
        {
			printf("write_dma_issue_q full\n");
            break;
		}

		struct list_head *lp = cmd->buf;

		for (int i = 0; i < cmd->nblock; i++)
        {
			set_auto_rx_dma(cmd->tag, i, (unsigned int)list_entry(lp, struct dma_buf, list)->buf_main,
					NVME_COMMAND_AUTO_COMPLETION_ON);
			lp = lp->next;
			dma_rx_tail++;
		}
		dprint("dma issued\n");

		q_pop_head(&write_dma_wait_q);
		q_push_tail(&write_dma_issue_q, cmd);
	}
	while (!q_empty(&read_dma_wait_q))
    {
		dprint("[hil_process_dmawait] read dma\n");

		struct cmd *cmd = q_get_head(&read_dma_wait_q);

		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);

		if (q_full(&read_dma_issue_q))
        {
			printf("read_dma_issue_q full\n");
            break;
		}

		struct list_head *lp = cmd->buf;

		for (int i = 0; i < cmd->nblock; i++)
        {
			set_auto_tx_dma(cmd->tag, i, (unsigned int)list_entry(lp, struct dma_buf, list)->buf_main,
					NVME_COMMAND_AUTO_COMPLETION_ON);
			lp = lp->next;
			dma_tx_tail++;
		}
		dprint("dma issued\n");

		q_pop_head(&read_dma_wait_q);
		q_push_tail(&read_dma_issue_q, cmd);
	}
}

void hil_process_dmadone(void)
{
	if (!q_empty(&write_dma_issue_q))
    {
		dprint("[hil proces dmadone] write\n");
		check_auto_rx_dma_done();

		while (!q_empty(&write_dma_issue_q))
        {
			struct cmd *cmd = q_get_head(&write_dma_issue_q);
            if (q_full(&ftl_wait_q))
            {
                printf("ftl_wait_q full");
                break;
            }
			q_pop_head(&write_dma_issue_q);
			q_push_tail(&ftl_wait_q, cmd);
		}
	}

	if (!q_empty(&read_dma_issue_q))
    {
		dprint("[hil proces dmadone] read\n");
		check_auto_tx_dma_done();

		while (!q_empty(&read_dma_issue_q))
        {
			struct cmd *cmd = q_get_head(&read_dma_issue_q);

			q_pop_head(&read_dma_issue_q);

            dmabuf_put(cmd->buf, cmd->nblock);
            del_cmd(cmd);
		}
	}
}

void hil_process_doneq(void)
{
	while (!q_empty(&done_q))
    {
		dprint("[hil_process_doneq]\n");

		struct cmd *cmd = q_get_head(&done_q);

		dprint("tag: %d, lba: %u, cnt: %u\n", cmd->tag, cmd->lba, cmd->nblock);

		q_pop_head(&done_q);
		dmabuf_put(cmd->buf, cmd->nblock);
		del_cmd(cmd);

		dprint("delete cmd\n");
	}
}
