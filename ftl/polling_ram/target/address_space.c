#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "address_space.h"
#include "common/cosmos_plus_memory_map.h"
#include "common/debug.h"

void *HOST_IP_ADDR;

void *NSC_0_BASEADDR;
void *NSC_1_BASEADDR;
void *NSC_2_BASEADDR;
void *NSC_3_BASEADDR;
void *NSC_4_BASEADDR;
void *NSC_5_BASEADDR;
void *NSC_6_BASEADDR;
void *NSC_7_BASEADDR;

void *ADMIN_CMD_DRAM_DATA_BUFFER;

void *base_NVME;
void *base_NAND;
void *base_DMA;

static int mem_fd;

static void nvme_address_init(void)
{
	void *map_base, *virt_addr;
	off_t target;

	printf("### 1. NVMe Address Init!\n");

	target = NVME_START_ADDR;
	map_base = mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, target & ~(0x10000 - 1));
	if (map_base == MAP_FAILED)
		FATAL;
	printf("0x%lx Memory mapped at address %p.\n", target, map_base);
	fflush(stdout);

	virt_addr = map_base + (target & (0x10000 - 1));

	base_NVME = virt_addr;

	HOST_IP_ADDR = virt_addr;

	printf("base_NVME: %p\n", base_NVME);
	printf("NVME_STATUS_REG_ADDR: %p\n", NVME_STATUS_REG_ADDR);
	printf("NVME_CMD_SRAM_ADDR: %p\n", NVME_CMD_SRAM_ADDR);
	puts("");
}


static void nand_address_init(void)
{
	void *map_base, *virt_addr;
	off_t target;

	printf("### 2. NAND Address Init!\n");

	target = NAND_START_ADDR;
	map_base = mmap(0, 0x80000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, target & ~(0x80000 - 1));
	if (map_base == MAP_FAILED)
		FATAL;
	printf("0x%lx Memory mapped at address %p.\n", target, map_base);
	fflush(stdout);

	base_NAND = virt_addr = map_base + (target & (0x80000 - 1));

	NSC_0_BASEADDR = virt_addr;
	NSC_1_BASEADDR = virt_addr + 0x10000;
	NSC_2_BASEADDR = virt_addr + 0x20000;
	NSC_3_BASEADDR = virt_addr + 0x30000;
	NSC_4_BASEADDR = virt_addr + 0x40000;
	NSC_5_BASEADDR = virt_addr + 0x50000;
	NSC_6_BASEADDR = virt_addr + 0x60000;
	NSC_7_BASEADDR = virt_addr + 0x70000;

	printf("base_NAND: %p\n\n", base_NAND);

	printf("NSC_0_BASEADDR: %p\n", NSC_0_BASEADDR);
	printf("NSC_1_BASEADDR: %p\n", NSC_1_BASEADDR);
	printf("NSC_2_BASEADDR: %p\n", NSC_2_BASEADDR);
	printf("NSC_3_BASEADDR: %p\n", NSC_3_BASEADDR);
	printf("NSC_4_BASEADDR: %p\n", NSC_4_BASEADDR);
	printf("NSC_5_BASEADDR: %p\n", NSC_5_BASEADDR);
	printf("NSC_6_BASEADDR: %p\n", NSC_6_BASEADDR);
	printf("NSC_7_BASEADDR: %p\n", NSC_7_BASEADDR);
	puts("");
}

static void dma_address_init(void)
{
	void *map_base;
	off_t target;

	printf("### 3. DMA Address Init!\n");

	target = DMA_MEMORY_START_ADDR;
	map_base = mmap(0, DMA_MEMORY_BUFFER_SIZE + DMA_MEMORY_ADMIN_SIZE,
			PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, target);
	if (map_base == MAP_FAILED)
		FATAL;
	printf("0x%lx Memory mapped at address %p.\n", target, map_base);
	fflush(stdout);

	base_DMA = map_base;

	ADMIN_CMD_DRAM_DATA_BUFFER = map_base + DMA_MEMORY_BUFFER_SIZE; // for admin_identify

	printf("base_DMA: %p\n", base_DMA);
	printf("ADMIN_CMD_DRAM_DATA_BUFFER: %p\n", ADMIN_CMD_DRAM_DATA_BUFFER);
	puts("");
}

void address_space_init(void)
{
	if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		perror("mem open");
		exit(1);
	}
	nvme_address_init();
	nand_address_init();
	dma_address_init();
}
