#ifndef HOST_LLD_H
#define HOST_LLD_H

#include "common/address_space.h"
#include "common/cosmos_plus_memory_map.h"

#define DMA_PHY(addr) ((void *)addr - (void *)base_DMA + (void *)DMA_MEMORY_START_ADDR)

#define HOST_DMA_DIRECT_TYPE				(1)
#define HOST_DMA_AUTO_TYPE				(0)

#define HOST_DMA_TX_DIRECTION				(1)
#define HOST_DMA_RX_DIRECTION				(0)

#define ONLY_CPL_TYPE					(0)
#define AUTO_CPL_TYPE					(1)
#define CMD_SLOT_RELEASE_TYPE				(2)

#pragma pack(push, 1)

typedef struct _DEV_IRQ_REG {
	union {
		unsigned int dword;
		struct {
			unsigned int pcieLink		:1;
			unsigned int busMaster		:1;
			unsigned int pcieIrq		:1;
			unsigned int pcieMsi		:1;
			unsigned int pcieMsix		:1;
			unsigned int nvmeCcEn		:1;
			unsigned int nvmeCcShn		:1;
			unsigned int mAxiWriteErr	:1;
			unsigned int mAxiReadErr	:1;
			unsigned int pcieMreqErr	:1;
			unsigned int pcieCpldErr	:1;
			unsigned int pcieCpldLenErr	:1;
			unsigned int reserved0		:20;
		};
	};
} DEV_IRQ_REG;

typedef struct _PCIE_STATUS_REG {
	union {
		unsigned int dword;
		struct {
			unsigned int ltssm		:6;
			unsigned int reserved0		:2;
			unsigned int pcieLinkUp		:1;
			unsigned int reserved1		:23;
		};
	};
} PCIE_STATUS_REG;

typedef struct _PCIE_FUNC_REG {
	union {
		unsigned int dword;
		struct {
			unsigned int busMaster		:1;
			unsigned int msiEnable		:1;
			unsigned int msixEnable		:1;
			unsigned int irqDisable		:1;
			unsigned int msiVecNum		:3;
			unsigned int reserved0		:25;
		};
	};
} PCIE_FUNC_REG;

// offset: 0x00000200, size: 4
typedef struct _NVME_STATUS_REG {
	union {
		unsigned int dword;
		struct {
			unsigned int ccEn		:1;
			unsigned int ccShn		:2;
			unsigned int reserved0		:1;
			unsigned int cstsRdy		:1;
			unsigned int cstsShst		:2;
			unsigned int reserved1		:25;
		};
	};
} NVME_STATUS_REG;

// offset: 0x00000300, size: 4
typedef struct _NVME_CMD_FIFO_REG {
	union {
		unsigned int dword;
		struct {
			unsigned int qID			:4;	//dy, Command QID, 0:admin, 1: IO
			unsigned int reserved0			:4;
			unsigned int cmdSlotTag			:7;	//dy, index of command slot, base NVME_CMD_SRAM_ADDR, 64byte each
			unsigned int reserved2			:1;
			unsigned int cmdSeqNum			:8;
			unsigned int reserved3			:7;
			unsigned int cmdValid			:1;
		};
	};
} NVME_CMD_FIFO_REG;

//offset: 0x00000304, size: 8
typedef struct _NVME_CPL_FIFO_REG {
	union {
		unsigned int dword[3];
		struct {
			struct 
			{
				unsigned int cid			:16;
				unsigned int sqId			:4;
				unsigned int reserved0			:12;
			};

			unsigned int specific;

			unsigned short cmdSlotTag			:7;
			unsigned short reserved1			:7;
			unsigned short cplType				:2;

			union {
				unsigned short statusFieldWord;
				struct 
				{
					unsigned short reserved0	:1;
					unsigned short SC		:8;
					unsigned short SCT		:3;
					unsigned short reserved1	:2;
					unsigned short MORE		:1;
					unsigned short DNR		:1;
				}statusField;
			};
		};
	};
} NVME_CPL_FIFO_REG;

//offset: 0x0000021C, size: 4
typedef struct _NVME_ADMIN_QUEUE_SET_REG {
	union {
		unsigned int dword;
		struct {
			unsigned int cqValid			:1;
			unsigned int sqValid			:1;
			unsigned int cqIrqEn			:1;
			unsigned int reserved0			:29;
		};
	};
} NVME_ADMIN_QUEUE_SET_REG;

//offset: 0x00000220, size: 8
typedef struct _NVME_IO_SQ_SET_REG {
	union {
		unsigned int dword[2];
		struct {
			unsigned int pcieBaseAddrL;
			unsigned int pcieBaseAddrH		:4;
			unsigned int reserved0			:11;
			unsigned int valid			:1;
			unsigned int cqVector			:4;
			unsigned int reserved1			:4;
			unsigned int sqSize			:8;
		};
	};
} NVME_IO_SQ_SET_REG;


//offset: 0x00000260, size: 8
typedef struct _NVME_IO_CQ_SET_REG {
	union {
		unsigned int dword[2];
		struct {
			unsigned int pcieBaseAddrL;
			unsigned int pcieBaseAddrH		:4;
			unsigned int reserved0			:11;
			unsigned int valid			:1;
			unsigned int irqVector			:3;
			unsigned int irqEn			:1;
			unsigned int reserved1			:4;
			unsigned int cqSize			:8;
		};
	};
} NVME_IO_CQ_SET_REG;

//offset: 0x00000204, size: 4
typedef struct _HOST_DMA_FIFO_CNT_REG
{
	union {
		unsigned int dword;
		struct 
		{
			unsigned char directDmaRx;
			unsigned char directDmaTx;
			unsigned char autoDmaRx;	// dy, DMA index for Rx, DMA done/request index, MAX 255, increment only
			unsigned char autoDmaTx;	// dy, DMA index for Tx, DMA done/request index, MAX 255, increment only
		};
	};
} HOST_DMA_FIFO_CNT_REG;

//offset: 0x0000030C, size: 16
typedef struct _HOST_DMA_CMD_FIFO_REG {
	union {
		unsigned int dword[4];
		struct 
		{
			unsigned int devAddr;				// dy, Memory Address
			unsigned int pcieAddrH;				// dy,
			unsigned int pcieAddrL;				// dy,
			struct 
			{
				unsigned int dmaLen		:13;
				unsigned int autoCompletion	:1;	// dy, NVME_COMMAND_AUTO_COMPLETION_ON or OFF
				unsigned int cmd4KBOffset	:9;	// dy, 4KB offset in a request
				unsigned int cmdSlotTag		:7;
				unsigned int dmaDirection	:1;	// dy, HOST_DMA_TX_DIRECTION, HOST_DMA_RX_DIRECTION
				unsigned int dmaType		:1;	// dy, HOST_DMA_AUTO_TYPE or HOST_DMA_DIRECT_TYPE
			};
		};
	};
} HOST_DMA_CMD_FIFO_REG;

//offset: 0x00002000, size: 64 * 128
typedef struct _NVME_CMD_SRAM {
	unsigned int dword[128][16];
} _NVME_CMD_SRAM;

#pragma pack(pop)


typedef struct _HOST_DMA_STATUS {
	HOST_DMA_FIFO_CNT_REG fifoHead;		// dy, DMA IP SFR. HW update
	HOST_DMA_FIFO_CNT_REG fifoTail;		// dy, Insert Issue DMA, tail insert head, FW update
	unsigned int directDmaTxCnt;
	unsigned int directDmaRxCnt;
	unsigned int autoDmaTxCnt;		// dy, Total auto DMA Tx Count, (cumulative)
	unsigned int autoDmaRxCnt;		// dy, Total auto DMA Rx Count, (cumulative)
} HOST_DMA_STATUS;


typedef struct _HOST_DMA_ASSIST_STATUS {
	unsigned int autoDmaTxOverFlowCnt;
	unsigned int autoDmaRxOverFlowCnt;
} HOST_DMA_ASSIST_STATUS;

void dev_irq_init();

void dev_irq_handler();

unsigned int check_nvme_cc_en();

void set_nvme_csts_rdy(unsigned int rdy);

void set_nvme_csts_shst(unsigned int shst);

void set_nvme_admin_queue(unsigned int sqValid, unsigned int cqValid, unsigned int cqIrqEn);

unsigned int get_nvme_cmd(unsigned short *qID, unsigned short *cmdSlotTag, unsigned int *cmdSeqNum, unsigned int *cmdDword);

void set_auto_nvme_cpl(unsigned int cmdSlotTag, unsigned int specific, unsigned int statusFieldWord);

void set_nvme_slot_release(unsigned int cmdSlotTag);

void set_nvme_cpl(unsigned int sqId, unsigned int cid, unsigned int specific, unsigned int statusFieldWord);

void set_io_sq(unsigned int ioSqIdx, unsigned int valid, unsigned int cqVector, unsigned int qSzie, unsigned int pcieBaseAddrL, unsigned int pcieBaseAddrH);

void set_io_cq(unsigned int ioCqIdx, unsigned int valid, unsigned int irqEn, unsigned int irqVector, unsigned int qSzie, unsigned int pcieBaseAddrL, unsigned int pcieBaseAddrH);

void set_direct_tx_dma(unsigned int devAddr, unsigned int pcieAddrH, unsigned int pcieAddrL, unsigned int len);

void set_direct_rx_dma(unsigned int devAddr, unsigned int pcieAddrH, unsigned int pcieAddrL, unsigned int len);

void set_auto_tx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion);

void set_auto_rx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion);

void check_direct_tx_dma_done();

void check_direct_rx_dma_done();

void check_auto_tx_dma_done();

void check_auto_rx_dma_done();

unsigned int check_auto_tx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex);

unsigned int check_auto_rx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex);

extern HOST_DMA_STATUS g_hostDmaStatus;
extern HOST_DMA_ASSIST_STATUS g_hostDmaAssistStatus;

#endif	/* HOST_LLD_H */
