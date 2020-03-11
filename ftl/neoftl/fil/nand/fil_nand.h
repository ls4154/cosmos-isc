#ifndef __FIL_NAND_H__
#define __FIL_NAND_H__

//#include "fil.h"
#include "nsc_driver.h"

typedef enum
{
    NAND_RESULT_SUCCESS,
    //NAND_RESULT_CMD_FAIL,        // COMMAND FAIL
    NAND_RESULT_UECC,            // Uncorrectable ECC
    NAND_RESULT_OP_RUNNING,    // operation running status
    NAND_RESULT_DONE,        // operation done
} NAND_RESULT;

void NAND_Initialize(void);
//void NAND_InitChCtlReg(void);
//void NAND_InitNandArray(void);

NAND_RESULT NAND_IssueRead(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bECCOn, int bSync);
NAND_RESULT NAND_ProcessRead(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bECCOn, int bSync);
NAND_RESULT NAND_IssueProgram(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bSync);
NAND_RESULT NAND_ProcessProgram(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bSync);
NAND_RESULT NAND_IssueErase(int nPCh, int nPWay, int nBlock, int bSync);
NAND_RESULT NAND_ProcessErase(int nPCh, int nPWay, int nBlock, int bSync);

#endif    // end of #ifndef __FIL_H__
