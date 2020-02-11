#include "common/debug.h"

#include "nvme/host_lld.h"
#include "hil.h"

#include "common/list.h"

#include "common/cosmos_types.h"
#include "common/cosmos_plus_system.h"

unsigned int storageCapacity_L;

void HIL_Initialize(void)
{
	DEBUG_ASSERT(LOGICAL_PAGE_SIZE == HOST_BLOCK_SIZE);	// if this is not same, HIL must convert block number to LPN
}
void HIL_Format(void)
{
}

void HIL_Run(void)
{
}

void HIL_SetStorageBlocks(unsigned int nStorageBlocks)
{
	storageCapacity_L = nStorageBlocks;
	ASSERT(storageCapacity_L > 0);
}

unsigned int HIL_GetStorageBlocks(void)
{
	return storageCapacity_L;
}

void HIL_ReadBlock(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount)
{
}

void HIL_WriteBlock(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount)
{
}

