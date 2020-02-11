#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address_space.h"

#include "hil/hil.h"
#include "hil/nvme/nvme_main.h"
#include "hil/nvme/host_lld.h"

#include "common/dma_memory.h"
#include "common/debug.h"

#include "ftl/ftl.h"
#include "fil/fil.h"

typedef void (*run_func_t)(void);
static run_func_t runs[] = {
	nvme_run,
	ftl_run,
	fil_run,
	NULL
};

#ifdef FTL_MULTI_THREAD
void *th_func(void *args)
{
	run_func_t f = args;
	while (1)
		f();
	return NULL;
}
#endif

int main(int argc, char **argv)
{
	address_space_init();

	dma_memory_init();

	dev_irq_init();

	ftl_init();
	fil_init();

	printf("Turn on the host PC.\n");

#ifdef FTL_MULTI_THREAD
	pthread_t tid;
	for (run_func_t *f = runs + 1; *f; f++) {
		int err;
		err = pthread_create(&tid, NULL, th_func, *f);
		if (err)
			FATAL;
	}
	(*runs)();
#else
	while (1)
		for (run_func_t *f = runs; *f; f++)
			(*f)();
#endif

	printf("Done\n");

	return 0;
}
