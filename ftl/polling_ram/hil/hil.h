#ifndef __HIL_H__
#define __HIL_H__

#include "common/list.h"
#include "nvme/nvme.h"
#include "common/cosmos_types.h"

// TODO, HIL BUFFER HIT

extern unsigned int storageCapacity_L;

void HIL_Initialize(void);
void HIL_Format(void);

void HIL_ReadBlock(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount);
void HIL_WriteBlock(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount);
void HIL_Run(void);

void HIL_SetStorageBlocks(unsigned int nStorageBlocks);
unsigned int HIL_GetStorageBlocks(void);

#endif
