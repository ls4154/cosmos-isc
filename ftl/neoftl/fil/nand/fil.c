#include "fil/fil.h"
#include "fil_nand.h"

#include "common/cmd_internal.h"
#include "common/cosmos_plus_system.h"
#include "common/dma_memory.h"

#include "util/debug.h"

#include <stdio.h>
#include <stdlib.h>

struct list_head request_q[USER_CHANNELS][USER_WAYS];

void fil_init(void)
{
    printf("\t\tNAND_Initialize - Start\n");
    NAND_Initialize();
    printf("\t\tNAND_Initialize - Finish\n");

    for (int i = 0; i < USER_CHANNELS; i++)
    {
        for (int j = 0; j < USER_WAYS; j++)
        {
            INIT_LIST_HEAD(&request_q[i][j]);
        }
    }
}

void fil_add_req(struct cmd *cmd, int type, void *buf_main, void *buf_spare, nand_addr_t addr)
{
    dindent(6);
    dprint("[fil_add_req]\n");

    struct nand_req *req = new_req();

    assert(req != NULL);

    req->type = type;
    req->addr = addr;
    req->cmd = cmd;
    req->buf_main = buf_main;
    req->buf_spare = buf_spare;

    list_add_tail(&req->list, &request_q[addr.ch][addr.way]);

    dindent(6);
    dprint("added to request_q\n");
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

                dindent(6);
                dprint("add_req (bufid:%d) %d/%d/%d/%d\n", db->id, db->addr.ch, db->addr.way,
                                        db->addr.block, db->addr.page);

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

static void fil_process_nand(void)
{
    for (int i = 0; i < USER_CHANNELS; i++)
    {
        for (int j = 0; j < USER_WAYS; j++)
        {
            if (list_empty(&request_q[i][j]))
            {
                continue;
            }

            /* dindent(6); */
            /* dprint("[process_request_queue] ch %d, way %d\n", i, j); */

            struct nand_req *req = list_first_entry(&request_q[i][j], struct nand_req, list);

            NAND_RESULT result = NAND_RESULT_OP_RUNNING;

            switch (req->type)
            {
                case NAND_TYPE_READ:
                    result = NAND_ProcessRead(i, j, req->addr.block, req->addr.page, req->buf_main, req->buf_spare, 1, 0);
                    break;
                case NAND_TYPE_PROGRAM:
                    result = NAND_ProcessProgram(i, j, req->addr.block, req->addr.page, req->buf_main, req->buf_spare, 0);
                    break;
                case NAND_TYPE_ERASE:
                    result = NAND_ProcessErase(i, j, req->addr.block, 0);
                    break;
                default:
                    ASSERT(0);
            }

            if (result == NAND_RESULT_DONE)
            {
                struct cmd *cmd = req->cmd;

                dindent(6);
                dprint("nand done %d/%d/%d/%d\n", i, j, req->addr.block, req->addr.page);

                if (req->type != NAND_TYPE_ERASE)
                {
                    cmd->ndone++;

                    dindent(6);
                    dprint("tag: %d lba %u cnt %u (%u/%u)\n", cmd->tag, cmd->lba, cmd->nblock, cmd->ndone, cmd->nblock);
                    if (cmd->ndone >= cmd->nblock)
                    {
                        dindent(6);
                        if (cmd->type == CMD_TYPE_RD)
                        {
                            ASSERT(!q_full(&read_dma_wait_q));
                            q_push_tail(&read_dma_wait_q, cmd);
                            dprint("rd cmd done\n");
                        }
                        else
                        {
                            ASSERT(!q_full(&done_q));
                            q_push_tail(&done_q, cmd);
                            dprint("wr cmd done\n");
                        }
                    }
                }

                list_del(&req->list);
                del_req(req);
            }
        }
    }
}

void fil_run(void)
{
    fil_process_wait();
    fil_process_nand();
}
