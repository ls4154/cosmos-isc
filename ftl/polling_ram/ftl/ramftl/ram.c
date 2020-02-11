#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "ftl/ftl.h"
#include "hil/nvme/nvme.h"

#include "hil/hil.h"
#include "hil/nvme/host_lld.h"

#include "common/list.h"

#include "mempool.h"

#define LIST_SIZE 2048
#define IO_TYPE_RD 0
#define IO_TYPE_WR 1

static void *disk;
static int disk_size = (1 << 26); // default 64 MiB

struct cmd_struct {
	int tag;
	int type;
	int nblock;
	int done;
	void *buf;
	struct list_head list;
};

struct io_entry {
	int type;
	struct cmd_struct *cmd;
	void *disk_block;
	void *buf;
};

static LIST_HEAD(cmd_list);

static struct io_entry list_io[LIST_SIZE];
static volatile int list_io_head;
static volatile int list_io_tail;

static inline int list_io_empty()
{
	return list_io_head == list_io_tail;
}
static inline int list_io_full()
{
	return list_io_head == (list_io_tail + 1) % LIST_SIZE;
}
static inline void list_io_move_head()
{
	list_io_head = (list_io_head + 1) % LIST_SIZE;
}
static inline void list_io_move_tail()
{
	list_io_tail = (list_io_tail + 1) % LIST_SIZE;
}

static void issue_read(struct cmd_struct *cmd, void *disk_block, void *buf)
{
	while (list_io_full());
	struct io_entry *ent = &list_io[list_io_tail];
	ent->type = IO_TYPE_RD;
	ent->cmd = cmd;
	ent->disk_block = disk_block;
	ent->buf = buf;
	__sync_synchronize();
	list_io_move_tail();
}

static void issue_write(struct cmd_struct *cmd, void *disk_block, void *buf)
{
	while (list_io_full());
	struct io_entry *ent = &list_io[list_io_tail];
	ent->type = IO_TYPE_WR;
	ent->cmd = cmd;
	ent->disk_block = disk_block;
	ent->buf = buf;
	__sync_synchronize();
	list_io_move_tail();
}

void *th_memcpy(void *arg)
{
	int err;
	sigset_t set;
	fprintf(stderr, "th_memcpy: thread start\n");

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);

	err = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (err) {
		perror("sigmask");
		exit(1);
	}

	while (1) {
		while (list_io_empty());
		//fprintf(stderr, "                                           copy start\n");

		struct io_entry *ent = &list_io[list_io_head];

		if (ent->type == IO_TYPE_RD)
			memcpy(ent->buf, ent->disk_block, 4096);
		else
			memcpy(ent->disk_block, ent->buf, 4096);

		ent->cmd->done++;
		//fprintf(stderr, "                                           copy done\n");

		list_io_move_head();
	}
}

static void *ram_disk_buf(unsigned int lba)
{
	if (lba * 4096 >= disk_size) {
		fprintf(stderr, "ram_disk_buf: LBA out of range\n");
		exit(1);
	}
	return disk + lba * 4096;
}

void ftl_init(void)
{
	int err;
	pthread_t tid;

	fprintf(stderr, "ftl_init: initializing ram disk\n");

	mempool_init();

	if ((disk = malloc(disk_size)) == NULL) {
		perror("malloc");
		exit(1);
	}
	memset(disk, 0xff, disk_size);
	memcpy(disk, "hello cosmos", 12);

	HIL_SetStorageBlocks(disk_size / 4096);


	err = pthread_create(&tid, NULL, &th_memcpy, NULL);
	if (err) {
		perror("pthread create");
		exit(1);
	}
	err = pthread_detach(tid);
	if (err) {
		perror("pthread detach");
		exit(1);
	}
}

void ftl_read(unsigned int tag, unsigned int nlb, unsigned int start_lba)
{
	unsigned int i;
	void *disk_block, *buf;

	fprintf(stderr, "ftl_read: tag: %u, nlb: %u, lba: %u\n", tag, nlb, start_lba);

	struct cmd_struct *cmd = malloc(sizeof(struct cmd_struct));
	cmd->tag = tag;
	cmd->type = IO_TYPE_RD;
	cmd->nblock = nlb;
	cmd->done = 0;
	cmd->buf = mempool_dmabuf_get(nlb);

	list_add_tail(&cmd->list, &cmd_list);

	fprintf(stderr, "          add_tail\n");

	for (i = 0; i < nlb; i++) {
		disk_block = ram_disk_buf(start_lba + i);
		buf = cmd->buf + i * 4096;
		issue_read(cmd, disk_block, buf);
	}
	fprintf(stderr, "          issue_read\n");
}
void ftl_write(unsigned int tag, unsigned int nlb, unsigned int start_lba)
{
	unsigned int i;
	void *disk_block, *buf;

	fprintf(stderr, "ftl_write: tag: %u, nlb: %u, lba: %u\n", tag, nlb, start_lba);

	struct cmd_struct *cmd = malloc(sizeof(struct cmd_struct));
	cmd->tag = tag;
	cmd->type = IO_TYPE_WR;
	cmd->nblock = nlb;
	cmd->done = 0;
	cmd->buf = mempool_dmabuf_get(nlb);

	for (i = 0; i < nlb; i++)
		set_auto_rx_dma(cmd->tag, i, (unsigned int)(cmd->buf + i * 4096), NVME_COMMAND_AUTO_COMPLETION_ON);
	check_auto_rx_dma_done();

	/* fprintf(stderr, "           first 4bytes [%c%c%c%c]\n", */
	/*                         ((char *)cmd->buf)[0], */
	/*                         ((char *)cmd->buf)[1], */
	/*                         ((char *)cmd->buf)[2], */
	/*                         ((char *)cmd->buf)[3]); */

	fprintf(stderr, "           dma done\n");

	list_add_tail(&cmd->list, &cmd_list);

	fprintf(stderr, "           add_tail\n");

	for (i = 0; i < nlb; i++) {
		disk_block = ram_disk_buf(start_lba + i);
		buf = cmd->buf + i * 4096;
		issue_write(cmd, disk_block, buf);
	}
	fprintf(stderr, "          issue_write\n");
}

void ftl_run(void)
{
	int i;
	struct cmd_struct *cmd;

	if (list_empty(&cmd_list))
		return;

	cmd = list_first_entry(&cmd_list, struct cmd_struct, list);
	if (cmd->done < cmd->nblock)
		return;

	fprintf(stderr, "ftl_run: cmd done\n");
	fprintf(stderr, "         tag: %d, type: %c\n", cmd->tag, cmd->type == IO_TYPE_RD ? 'R' : 'W');
	/* fprintf(stderr, "           first 4bytes [%c%c%c%c]\n", */
	/*                         ((char *)cmd->buf)[0], */
	/*                         ((char *)cmd->buf)[1], */
	/*                         ((char *)cmd->buf)[2], */
	/*                         ((char *)cmd->buf)[3]); */

	if (cmd->type == IO_TYPE_RD) {
		for (i = 0; i < cmd->nblock; i++)
			set_auto_tx_dma(cmd->tag, i, (unsigned int)(cmd->buf + i * 4096), NVME_COMMAND_AUTO_COMPLETION_ON);
		check_auto_tx_dma_done();
		fprintf(stderr, "         dma done\n");
	}
	list_del(&cmd->list);
	mempool_put(cmd->nblock);
	free(cmd);
}
