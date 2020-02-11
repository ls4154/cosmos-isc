#ifndef ADDRESS_SPACE_H
#define ADDRESS_SPACE_H

extern void *HOST_IP_ADDR;

extern void *NSC_0_BASEADDR;
extern void *NSC_1_BASEADDR;
extern void *NSC_2_BASEADDR;
extern void *NSC_3_BASEADDR;
extern void *NSC_4_BASEADDR;
extern void *NSC_5_BASEADDR;
extern void *NSC_6_BASEADDR;
extern void *NSC_7_BASEADDR;

extern void *ADMIN_CMD_DRAM_DATA_BUFFER;

extern void *base_NVME;
extern void *base_NAND;
extern void *base_DMA;

#define DEV_IRQ_MASK_REG_ADDR				(HOST_IP_ADDR + 0x4)	//dy, interrupt mask, layout DEV_IRQ_REG
#define DEV_IRQ_CLEAR_REG_ADDR				(HOST_IP_ADDR + 0x8)
#define DEV_IRQ_STATUS_REG_ADDR				(HOST_IP_ADDR + 0xC)	// dy, DEV_IRQ_REG

#define PCIE_STATUS_REG_ADDR				(HOST_IP_ADDR + 0x100)
#define PCIE_FUNC_REG_ADDR				(HOST_IP_ADDR + 0x104)

#define NVME_STATUS_REG_ADDR				(HOST_IP_ADDR + 0x200)	// dy, NVME_STATUS_REG
#define HOST_DMA_FIFO_CNT_REG_ADDR			(HOST_IP_ADDR + 0x204)
#define NVME_ADMIN_QUEUE_SET_REG_ADDR			(HOST_IP_ADDR + 0x21C)
#define NVME_IO_SQ_SET_REG_ADDR				(HOST_IP_ADDR + 0x220)
#define NVME_IO_CQ_SET_REG_ADDR				(HOST_IP_ADDR + 0x260)

#define NVME_CMD_FIFO_REG_ADDR				(HOST_IP_ADDR + 0x300)
#define NVME_CPL_FIFO_REG_ADDR				(HOST_IP_ADDR + 0x304)
#define HOST_DMA_CMD_FIFO_REG_ADDR			(HOST_IP_ADDR + 0x310)

#define NVME_CMD_SRAM_ADDR				(HOST_IP_ADDR + 0x2000)

void address_space_init(void);

#endif
