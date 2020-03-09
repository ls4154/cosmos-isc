#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fil/fil.h"

#include "hil/hil.h"

#include "common/cmd_internal.h"

#include "util/debug.h"

#define HOST_BLOCK_SIZE 4096

static void *disk;

#define USER_CHANNELS 8
#define USER_WAYS 8
struct list_head nand_q[USER_CHANNELS][USER_WAYS];

void fil_init(void)
{
    unsigned int disk_size = hil_get_storage_blocks() * HOST_BLOCK_SIZE;

    printf("fil init: disk size %u\n", disk_size);

    for (int i = 0; i < USER_CHANNELS; i++)
    {
        for (int j = 0; j < USER_WAYS; j++)
        {
            INIT_LIST_HEAD(&nand_q[i][j]);
        }
    }

    disk = malloc(disk_size);
    if (disk == NULL)
    {
        perror("malloc disk");
        exit(1);
    }
    memset(disk, 0xFF, disk_size);
    printf("start %p, end %p\n", disk, disk + disk_size);
}

static void fil_add_req(struct cmd *cmd, int type, void *buf_main, void *buf_spare, nand_addr_t addr)
{
        dindent(6);
        dprint("[fil_add_req]\n");

        struct nand_req *req = malloc(sizeof(struct nand_req)); //TODO use req pool
        req->type = type;
        req->addr = addr;
        req->cmd = cmd;
        req->buf_main = buf_main;
        req->buf_spare = buf_spare;

        list_add_tail(&req->list, &nand_q[addr.ch][addr.way]);

        dindent(6);
        dprint("added to nand_q\n");
}

static void fil_process_nand(void)
{
    while (!list_empty(&nand_q[0][0]))
    {
        dindent(6);
        dprint("[fil_nand]\n");

        struct nand_req *req = list_first_entry(&nand_q[0][0], struct nand_req, list);
        struct cmd *cmd = req->cmd;
        dindent(6);
        dprint("tag: %d lba %u cnt %u\n", cmd->tag, cmd->lba, cmd->nblock);
        dindent(6);
        dprint("buf: %p, disk %p\n", req->buf_main, disk + HOST_BLOCK_SIZE * req->addr.page);

        if (req->type == NAND_TYPE_READ)
        {
            memcpy(req->buf_main, disk + HOST_BLOCK_SIZE * req->addr.page, HOST_BLOCK_SIZE);

            dindent(6);
            dprint("rd copy done\n");
        }
        else
        {
            memcpy(disk + HOST_BLOCK_SIZE * req->addr.page, req->buf_main, HOST_BLOCK_SIZE);

            dindent(6);
            dprint("wr copy done\n");
        }

        list_del(&req->list);
        free(req);

        if (cmd->ndone == 0)
        {
            dindent(6);
            dprint("[%d] [%d]\n", ((char *)req->buf_main)[0], ((char *)disk + HOST_BLOCK_SIZE * req->addr.page)[0]);
        }

        cmd->ndone++;

        dindent(6);
        dprint("%u/%u\n", cmd->ndone, cmd->nblock);

        if (cmd->ndone < cmd->nblock)
            continue;

        dindent(6);
        if (cmd->type == CMD_TYPE_RD)
        {
            dprint("rd cmd done\n");
            q_push_tail(&read_dma_wait_q, cmd);
        }
        else
        {
            dprint("wr cmd done\n");
            q_push_tail(&done_q, cmd);
        }

        break;
    }
}
static void fil_process_wait(void)
{
    if (!q_empty(&nand_wait_q))
    {
        struct cmd *cmd = q_get_head(&nand_wait_q);

        dindent(6);
        dprint("[ftl_process_wait]\n");
        dindent(6);
        dprint("tag %d, %c, lba:%u, cnt:%u\n", cmd->tag, cmd->type == CMD_TYPE_RD ? 'R' : 'W',
                                               cmd->lba, cmd->nblock);

        if (cmd->type == CMD_TYPE_RD)
        {
            struct list_head *lp = cmd->buf;
            for (int i = 0; i < cmd->nblock; i++)
            {
                struct dma_buf *db = list_entry(lp, struct dma_buf, list);

                fil_add_req(cmd, NAND_TYPE_READ, db->buf_main, db->buf_spare, db->addr);

                lp = lp->next;
            }
        }
        else
        {
            struct list_head *lp = cmd->buf;
            for (int i = 0; i < cmd->nblock; i++)
            {
                struct dma_buf *db = list_entry(lp, struct dma_buf, list);

                fil_add_req(cmd, NAND_TYPE_WRITE, db->buf_main, db->buf_spare, db->addr);

                lp = lp->next;
            }
        }

        q_pop_head(&nand_wait_q);
    }
}

void fil_run(void)
{
    fil_process_wait();
    fil_process_nand();
}

