#ifndef FMC_DRIVER_H
#define FMC_DRIVER_H

#include "common/cosmos_plus_system.h"

#define V2FCommand_NOP 0
#define V2FCommand_Reset 1
#define V2FCommand_SetFeatures 6
#define V2FCommand_GetFeatures 46
#define V2FCommand_ReadPageTrigger 13
#define V2FCommand_ReadPageTransfer 18
#define V2FCommand_ProgramPage 28
#define V2FCommand_BlockErase 37
#define V2FCommand_StatusCheck 41
#define V2FCommand_ReadPageTransferRaw 55

#define V2FCrcValid(errorInformation) !!((errorInformation) & (0x10000000))
#define V2FSpareChunkValid(errorInformation) !!((errorInformation) & (0x01000000))
#define V2FPageChunkValid(errorInformation) ((errorInformation) == 0xffffffff)
#define V2FWorstChunkErrorCount(errorInformation) (((errorInformation) & 0x00ff0000) >> 16)

#define V2FEnterToggleMode(dev, way, payLoadAddr) V2FSetFeaturesSync(dev, way, 0x00000006, 0x00000008, 0x00000020, payLoadAddr)

#define V2FWayReady(readyBusy, wayNo) (((readyBusy) >> (wayNo)) & 1)	// dy, way ready bitmap, 1b: ready, 0b: busy
#define V2FTransferComplete(completeFlag) ((completeFlag) & 1)
#define V2FRequestReportDone(statusReport) ((statusReport) & 1)
#define V2FEliminateReportDoneFlag(statusReport) ((statusReport) >> 1)
#define V2FRequestComplete(statusReport) (((statusReport) & 0x60) == 0x60)
#define V2FRequestFail(statusReport) ((statusReport) & 3)

extern void *base_NVME;
extern void *base_NAND;
extern void *base_DMA;

// #define NVME_PHY(addr) ((void *)addr - base_NVME + 0x83c00000)
// #define NAND_PHY(addr) ((void *)addr - base_NAND + 0x43c00000)
#define DMA_PHY(addr) ((char *)addr - (char *)base_DMA + (char *)0x30000000)

typedef struct {
	unsigned int cmdSelect;			// dy, NAND command, V2FCommand_ReadPageTrigger.
	unsigned int rowAddress;		// dy, IO adress
	unsigned int userData;
	unsigned int dataAddress;
	unsigned int spareAddress;
	unsigned int errorCountAddress;		// dy, read error information, ERROR_INFO_FAIL, ERROR_INFO_PASS, ERROR_INFO_WARNING
	unsigned int completionAddress;
	unsigned int waySelection;		// dy, channel ??? way
	unsigned int channelBusy;		// dy, 1: busy, 0: idle, HW Simulator.
	unsigned int readyBusy;			// dy, channel way ready bitmap, 1b: ready, 0b: busy
} V2FMCRegisters;

unsigned int V2FIsControllerBusy(V2FMCRegisters* dev);
void V2FResetSync(V2FMCRegisters* dev, int way);
void V2FSetFeaturesSync(V2FMCRegisters* dev, int way, unsigned int feature0x02,
			unsigned int feature0x10, unsigned int feature0x01, unsigned int payLoadAddr);
void V2FGetFeaturesSync(V2FMCRegisters* dev, int way, unsigned int* feature0x01,
			unsigned int* feature0x02, unsigned int* feature0x10, unsigned int* feature0x30);
void V2FReadPageTriggerAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress);
void V2FReadPageTransferAsync(V2FMCRegisters* dev, int way, void* pageDataBuffer, void* spareDataBuffer,
				unsigned int* errorInformation, unsigned int* completion, unsigned int rowAddress);
void V2FReadPageTransferRawAsync(V2FMCRegisters* dev, int way, void* pageDataBuffer, unsigned int* completion);
void V2FProgramPageAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress, void* pageDataBuffer,
				void* spareDataBuffer);
void V2FEraseBlockAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress);
void V2FStatusCheckAsync(V2FMCRegisters* dev, int way, unsigned int* statusReport);
unsigned int V2FStatusCheckSync(V2FMCRegisters* dev, int way);
unsigned int V2FReadyBusyAsync(V2FMCRegisters* dev);

#endif /* FMC_DRIVER_H */
