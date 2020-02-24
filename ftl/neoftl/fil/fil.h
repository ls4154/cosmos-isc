#ifndef FIL_H
#define FIL_H

#include "common/cmd_internal.h"

void fil_init(void);
void fil_run(void);

void fil_add_req(struct cmd *cmd, int type, void *buf, nand_addr_t addr);

#endif /* FIL_H */
