#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fil_nand.h"
#include "fil/fil.h"

#include "hil/hil.h"

#include "common/cmd_internal.h"

#include "util/debug.h"

#define HOST_BLOCK_SIZE 4096

void fil_init(void)
{
    unsigned int disk_size = hil_get_storage_blocks() * HOST_BLOCK_SIZE;

    printf("fil init: disk size %u\n", disk_size);

    fil_nand_init();
}

static void fil_process_wait(void)
{
    if (!q_empty(&nand_wait_q))
    {
        struct cmd *cmd = q_get_head(&nand_wait_q);

        dindent(6);
        dprint("[fil_process_wait]\n");
        dindent(6);
        dprint("tag %d, %c, lba:%u, cnt:%u\n", cmd->tag, cmd->type == CMD_TYPE_RD ? 'R' : 'W',
                                              cmd->lba, cmd->nblock);

        if (cmd->type == CMD_TYPE_RD)
        {
            struct list_head *lp = cmd->buf;
            for (int i = 0; i < cmd->nblock; i++)
            {
                struct dma_buf *db = list_entry(lp, struct dma_buf, list);

                if (!db->addr.valid)
                {
                    cmd->ndone++;
                }
                else
                {
                    fil_add_req(cmd, NAND_TYPE_READ, db->buf_main, db->buf_spare, db->addr);
                }

                lp = lp->next;
            }

            if (cmd->ndone == cmd->nblock)
            {
                q_push_tail(&read_dma_wait_q, cmd);
            }
        }
        else
        {
            struct list_head *lp = cmd->buf;
            for (int i = 0; i < cmd->nblock; i++)
            {
                struct dma_buf *db = list_entry(lp, struct dma_buf, list);

                if (db->addr.page == 0)
                {
                    fil_add_req(cmd, NAND_TYPE_ERASE, NULL, NULL, db->addr);
                }

                fil_add_req(cmd, NAND_TYPE_PROGRAM, db->buf_main, db->buf_spare, db->addr);

                lp = lp->next;
            }
        }

        q_pop_head(&nand_wait_q);
    }
}

void fil_run(void)
{
    fil_process_wait();
    fil_nand_run();
}
