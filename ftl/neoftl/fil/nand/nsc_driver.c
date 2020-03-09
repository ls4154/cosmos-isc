#include "common/cosmos_plus_system.h"
#include "nsc_driver.h"

unsigned int 
V2FIsControllerBusy(V2FMCRegisters* dev)
{
	volatile unsigned int channelBusy = *((volatile unsigned int*)&(dev->channelBusy));

	return channelBusy;
}

void
V2FResetSync(V2FMCRegisters* dev, int way)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_Reset;
	while (V2FIsControllerBusy(dev));
}

void
V2FSetFeaturesSync(V2FMCRegisters* dev, int way, unsigned int feature0x02, unsigned int feature0x10, unsigned int feature0x01, unsigned int payLoadAddr)
{
	unsigned int* payload = (unsigned int*)payLoadAddr;
	payload[0] = feature0x02;
	payload[1] = feature0x10;
	payload[2] = feature0x01;
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->userData)) = (unsigned int)DMA_PHY(payload);
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_SetFeatures;
	while (V2FIsControllerBusy(dev));
}

void
V2FGetFeaturesSync(V2FMCRegisters* dev, int way, unsigned int* feature0x01, unsigned int* feature0x02, unsigned int* feature0x10, unsigned int* feature0x30)
{
	volatile unsigned int buffer[4] = {0};
	volatile unsigned int completion = 0;
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->userData)) = (unsigned int)buffer;
	*((volatile unsigned int*)&(dev->completionAddress)) = (unsigned int)&completion;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_GetFeatures;
	while (V2FIsControllerBusy(dev));
	while (!(completion & 1));
	*feature0x01 = buffer[0];
	*feature0x02 = buffer[1];
	*feature0x10 = buffer[2];
	*feature0x30 = buffer[3];
}

void
V2FReadPageTriggerAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->rowAddress)) = rowAddress;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_ReadPageTrigger;
}

void
V2FReadPageTransferAsync(V2FMCRegisters* dev, int way, void* pageDataBuffer, void* spareDataBuffer, unsigned int* errorInformation, unsigned int* completion, unsigned int rowAddress)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->dataAddress)) = (unsigned int)DMA_PHY(pageDataBuffer);
	*((volatile unsigned int*)&(dev->spareAddress)) = (unsigned int)DMA_PHY(spareDataBuffer);
	*((volatile unsigned int*)&(dev->errorCountAddress)) = (unsigned int)DMA_PHY(errorInformation);
	*((volatile unsigned int*)&(dev->completionAddress)) = (unsigned int)DMA_PHY(completion);
	*((volatile unsigned int*)&(dev->rowAddress)) = rowAddress;
	*completion = 0;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_ReadPageTransfer;
}

void
V2FReadPageTransferRawAsync(V2FMCRegisters* dev, int way, void* pageDataBuffer, unsigned int* completion)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->dataAddress)) = (unsigned int)DMA_PHY(pageDataBuffer);
	*((volatile unsigned int*)&(dev->completionAddress)) = (unsigned int)DMA_PHY(completion);
	*completion = 0;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_ReadPageTransferRaw;
}


void
V2FProgramPageAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress, void* pageDataBuffer, void* spareDataBuffer)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->rowAddress)) = rowAddress;
	*((volatile unsigned int*)&(dev->dataAddress)) = (unsigned int)DMA_PHY(pageDataBuffer);
	*((volatile unsigned int*)&(dev->spareAddress)) = (unsigned int)DMA_PHY(spareDataBuffer);
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_ProgramPage;

}

void
V2FEraseBlockAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->rowAddress)) = rowAddress;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_BlockErase;
}

void
V2FStatusCheckAsync(V2FMCRegisters* dev, int way, unsigned int* statusReport)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->completionAddress)) = (unsigned int)DMA_PHY(statusReport);
	*statusReport = 0;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_StatusCheck;

}

unsigned int
V2FStatusCheckSync(V2FMCRegisters* dev, int way)
{
	volatile unsigned int status;
	V2FStatusCheckAsync(dev, way, (unsigned int*)&status);
	while (!(status & 1));
	return (status >> 1);
}

unsigned int
V2FReadyBusyAsync(V2FMCRegisters* dev)
{
	volatile unsigned int readyBusy = dev->readyBusy;

	return readyBusy;
}

