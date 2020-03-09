#include <stdio.h>

#include "ftl/ftl.h"

#include "hil/hil.h"
#include "common/cmd_internal.h"
#include "common/cosmos_plus_system.h"

#include "fil/fil.h"

#include "util/debug.h"

#define HOST_BLOCK_SIZE 4096
#define NAND_PAGE_SIZE 16384

#define HOST_BLOCK_CNT (16384 * 1024)
static int disk_size = 4096 * HOST_BLOCK_CNT; // default 64 GiB

static nand_addr_t l2p_table[HOST_BLOCK_CNT];

static unsigned int i_ch = 0;
static unsigned int i_way = 0;
static unsigned int i_block = 50;
static unsigned int i_page = 0;

void ftl_init(void)
{
    printf("ftl_init\n");
    hil_set_storage_blocks(HOST_BLOCK_CNT);
}

static void ftl_process_q(void)
{
    if (!q_empty(&ftl_wait_q) && !q_full(&nand_wait_q))
    {
        struct cmd *cmd = q_get_head(&ftl_wait_q);

        dindent(3);
        dprint("[ftl_process_q]\n");

        dindent(3);
        dprint("tag %d, %c, lba:%u, cnt:%u\n", cmd->tag, cmd->type == CMD_TYPE_RD ? 'R' : 'W',
                                               cmd->lba, cmd->nblock);
        if (cmd->type == CMD_TYPE_RD)
        {
            dindent(3);
            dprint("adding req rd\n");

            struct list_head *lp = cmd->buf;
            for (int i = 0; i < cmd->nblock; i++)
            {
                nand_addr_t *addr = &list_entry(lp, struct dma_buf, list)->addr;

                *addr = l2p_table[cmd->lba + i];

                lp = lp->next;
            }
        }
        else // CMD_TYPE_WR
        {
            dindent(3);
            dprint("adding req wr\n");
            struct list_head *lp = cmd->buf;
            for (int i = 0; i < cmd->nblock; i++)
            {
                nand_addr_t *addr = &list_entry(lp, struct dma_buf, list)->addr;

                l2p_table[cmd->lba + i].ch = i_ch;
                l2p_table[cmd->lba + i].way = i_way;
                l2p_table[cmd->lba + i].block = i_block;
                l2p_table[cmd->lba + i].page = i_page;
                l2p_table[cmd->lba + i].valid = 1;

                dindent(3);
                dprint("%d/%d/%d/%d\n", i_ch, i_way, i_block, i_page);

                i_ch++;
                if (i_ch >= USER_CHANNELS)
                {
                    i_ch = 0;
                    i_way++;
                    if (i_way >= USER_WAYS)
                    {
                        i_way = 0;
                        i_page++;
                        if (i_page >= PAGES_PER_BLOCK)
                        {
                            i_page = 0;
                            i_block++;
                            if (i_block >= 512) //TODO
                            {
                                assert(0);
                                printf("need gc\n");
                            }
                        }
                    }
                }

                *addr = l2p_table[cmd->lba + i];

                lp = lp->next;
            }
        }
        q_pop_head(&ftl_wait_q);
        q_push_tail(&nand_wait_q, cmd);
    }
}

void ftl_run(void)
{
    ftl_process_q();
}
