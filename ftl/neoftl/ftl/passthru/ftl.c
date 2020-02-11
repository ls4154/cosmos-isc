#include <stdio.h>

#include "ftl/ftl.h"

#include "hil/hil.h"
#include "common/cmd_internal.h"

static int disk_size = (1 << 25); // default 32 MiB

void ftl_init(void)
{
	printf("ftl_init: initializing\n");
	hil_set_storage_blocks(disk_size / 4096);
}
static void ftl_process_q(void)
{
	while (!q_empty(&ftl_wait_q)) {
		printf("------------------");
		printf("ftl_process_q\n");
		struct cmd *cmd = q_get_head(&ftl_wait_q);
		while (q_full(&nand_wait_q));
		q_push_tail(&nand_wait_q, cmd);
		q_pop_head(&ftl_wait_q);
	}
}
void ftl_run(void)
{
	ftl_process_q();
}
