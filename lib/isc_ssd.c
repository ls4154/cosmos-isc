#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "isc_ssd.h"

int isc_init_shared(void)
{
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd == -1) {
		perror("open mem\n");
		return 0;
	}
	off_t target = strtoul("0x30000000", 0, 0);
	void *map_base = mmap(0, 0x10000000, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, target & ~(0x10000000 - 1));
	if (map_base == MAP_FAILED) {
		perror("mmap base");
		return 0;
	}
	void *virt_addr = (char *)map_base + (target & (0x10000000 - 1));
	isc_shared = (Shared *)((char *)virt_addr + 0x8000000);
	return 0;
}

int isc_read_request(int pp, int lba, int bytes)
{
	if (isc_shared[pp].status != ISC_ST_IDLE)
		return -1;
	isc_shared[pp].opcode = ISC_OP_RD;
	isc_shared[pp].lba = lba;
	isc_shared[pp].sector_cnt = toSect(bytes);
	isc_shared[pp].status = ISC_ST_REQ;
	return 0;
}

int isc_write_request(int pp, int lba, int bytes)
{
	if (isc_shared[pp].status != ISC_ST_IDLE)
		return -1;
	isc_shared[pp].opcode = ISC_OP_WR;
	isc_shared[pp].lba = lba;
	isc_shared[pp].sector_cnt = toSect(bytes);
	isc_shared[pp].status = ISC_ST_REQ;
	return 0;
}
