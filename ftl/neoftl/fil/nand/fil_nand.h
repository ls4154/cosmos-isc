#ifndef FIL_NAND_H
#define FIL_NAND_H

#include "common/cmd_internal.h"

void fil_nand_init(void);

void fil_nand_run(void);

void fil_add_req(struct cmd *cmd, int type, void *buf_main, void *buf_spare, nand_addr_t addr);

#endif
