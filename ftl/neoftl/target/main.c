#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/address_space.h"
#include "common/cmd_internal.h"

#include "hil/hil.h"
#include "hil/host_lld.h"

#include "common/dma_memory.h"
#include "util/debug.h"

#include "ftl/ftl.h"
#include "fil/fil.h"

#include "run.h"

void tttt(void)
{
    hil_run();
    ftl_run();
}

LIST_RUN(init_list,
    address_space_init,
    dma_memory_init,
    dev_irq_init,
    cmd_internal_init,
    hil_init,
    ftl_init,
    fil_init
);

LIST_RUN(run_list,
    //hil_run,
    //ftl_run,
    tttt,
    fil_run
);

int main(int argc, char **argv)
{
    RUN_ONCE(init_list);

    printf("Turn on the host PC.\n");

    RUN(run_list);

    printf("Done\n");

    return 0;
}
