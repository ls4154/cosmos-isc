#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "common/cmd_internal.h"
#include "common/dma_memory.h"

#include "util/debug.h"

#include "nsc_driver.h"

#define HOST_BLOCK_SIZE 4096

//#define USER_CHANNELS 8
//#define USER_WAYS 8

#define ERROR_INFO_FAIL		0
#define ERROR_INFO_PASS		1
#define ERROR_INFO_WARNING	2

#define FIL_READ_RETRY (5) // re-try count when UECC

#define OPERATION_STEP_START (0)

typedef enum
{
    NAND_READ_STEP_NONE = OPERATION_STEP_START,
    NAND_READ_STEP_CHECK_READ_ISSUABLE,
    NAND_READ_STEP_ISSUE_READ,
    NAND_READ_STEP_WAIT_READ_DONE,
    NAND_READ_STEP_ISSUE_GET_STATUS_READ,
    NAND_READ_STEP_WAIT_GET_STATUS_READ_DONE,
    NAND_READ_STEP_CHECK_READ_RESULT,
    NAND_READ_STEP_CHECK_TRANSFER_ISSUABLE,
    NAND_READ_STEP_ISSUE_TRANSFER,
    NAND_READ_STEP_WAIT_TRASFER_DONE,
    NAND_READ_STEP_CHECK_TRANSFER_RESULT,
    NAND_READ_STEP_DONE,
    NAND_READ_STEP_COUNT
} NAND_READ_STEP;

typedef enum
{
    NAND_PGM_STEP_NONE = OPERATION_STEP_START,
    NAND_PGM_STEP_CHECK_PROGRAM_ISSUABLE,
    NAND_PGM_STEP_ISSUE_PROGRAM,
    NAND_PGM_STEP_WAIT_PROGRAM_DONE,
    NAND_PGM_STEP_ISSUE_GET_STATUS,
    NAND_PGM_STEP_WAIT_GET_STATUS_DONE,
    NAND_PGM_STEP_CHECK_STATUS,
    NAND_PGM_STEP_DONE,
    NAND_PGM_STEP_COUNT
} NAND_PROGRAM_STEP;

typedef enum
{
    NAND_ERASE_STEP_NONE = OPERATION_STEP_START,
    NAND_ERASE_STEP_CHECK_ERASE_ISSUABLE,
    NAND_ERASE_STEP_ISSUE_ERASE,
    NAND_ERASE_STEP_WAIT_ERASE_DONE,
    NAND_ERASE_STEP_ISSUE_GET_STATUS,
    NAND_ERASE_STEP_WAIT_GET_STATUS_DONE,
    NAND_ERASE_STEP_CHECK_STATUS,
    NAND_ERASE_STEP_DONE,
    NAND_ERASE_STEP_COUNT
} NAND_ERASE_STEP;

typedef struct
{
    union
    {
        NAND_READ_STEP      nRead;
        NAND_PROGRAM_STEP   nProgram;
        NAND_ERASE_STEP     nErase;
        unsigned int        nCommon;
    };
    unsigned int            nRetry;
} WAY_STATUS;

typedef struct
{
    unsigned int anData[USER_CHANNELS][USER_WAYS];
} NAND_COMPLETION;

typedef struct
{
    unsigned int anData[USER_CHANNELS][USER_WAYS];
} NAND_STATUS;

typedef struct
{
    unsigned int anData[USER_CHANNELS][USER_WAYS][ERROR_INFO_WORD_COUNT];
} NAND_ERROR;

V2FMCRegisters *nand_ctrl_reg[USER_CHANNELS];

static struct list_head request_q[USER_CHANNELS][USER_WAYS];

static WAY_STATUS way_status[USER_CHANNELS][USER_WAYS];

static NAND_COMPLETION *nand_completion;
static NAND_STATUS *nand_status;
static NAND_ERROR *nand_error;


static int check_bad_block(int pbn)
{
    static int bad_pbn[] =
    {
        527, 517, 513, 505,
        481, 415,
        330, 320,
        294, 292, 286, 256,
        5, 3, 1, 0,
    };

    for (int i = 0; i < sizeof(bad_pbn) / sizeof(int); i++)
    {
        if (bad_pbn[i] == pbn)
        {
            return 1;
        }
    }
    return 0;
}

static int check_addr_valid(int ch, int way, int block, int page)
{
    if ((ch >= USER_CHANNELS) || (way >= USER_WAYS) ||
            (block >= TOTAL_BLOCKS_PER_DIE) || (page >= PAGES_PER_MLC_BLOCK))
    {
        return 0;
    }

    return 1;
}

static int get_transfer_result(unsigned int *panErrorFlag)
{
    assert(0 == ERROR_INFO_FAIL);

    unsigned int nErrorInfo0 = panErrorFlag[0];
    unsigned int nErrorInfo1 = panErrorFlag[1];

    if (V2FCrcValid(nErrorInfo0))
    {
        if (V2FSpareChunkValid(nErrorInfo0))
        {
            if (V2FPageChunkValid(nErrorInfo1))
            {
                if (V2FWorstChunkErrorCount(nErrorInfo0) > BIT_ERROR_THRESHOLD_PER_CHUNK)
                {
                    return ERROR_INFO_WARNING;
                }

                return ERROR_INFO_PASS;
            }
        }
    }

    return ERROR_INFO_FAIL;
}

static unsigned int get_lun_base_addr(int nLUN)
{
    if (nLUN == 0)
    {
        return LUN_0_BASE_ADDR;
    }
    else
    {
        assert(nLUN == 1);
    }

    return LUN_1_BASE_ADDR;
}

static int check_way_idle(int ch, int way)
{
    unsigned int nReadyBusy;
    nReadyBusy = V2FReadyBusyAsync(nand_ctrl_reg[ch]);
    if (V2FWayReady(nReadyBusy, way))
    {
        return 1;
    }

    return 0;
}

static int check_issuable(int ch, int way)
{
    if (V2FIsControllerBusy(nand_ctrl_reg[ch]))
    {
        return 0;
    }

    if (!check_way_idle(ch, way))
    {
        return 0;
    }

    return 1;
}

static unsigned int get_row_addr(int ch, int way, int block, int page)
{
    // unsigned int nDieNo;
    unsigned int nLUN;                // Logical Unit Number,    Die in a chip
    unsigned int nLocalBlockNo;        //    BlockNo in a Logical Unit
    unsigned int row_addr;

    // nDieNo = PhyChWway2VDie(ch, way);
    nLUN = block / TOTAL_BLOCKS_PER_LUN;
    nLocalBlockNo = (nLUN * TOTAL_BLOCKS_PER_LUN) + (block % TOTAL_BLOCKS_PER_LUN);

    row_addr = get_lun_base_addr(nLUN) + nLocalBlockNo * PAGES_PER_MLC_BLOCK + page;

    return row_addr;
}

static int check_request_complete(unsigned int nResult)
{
    const unsigned int REQUEST_FAIL_MASK = 0xC0;
    if (nResult & REQUEST_FAIL_MASK)
    {
        return 1;
    }

    return 0;
}

static int _IsRequestFail(unsigned int nResult)
{
    const unsigned int REQUEST_FAIL_MASK = 0x6;
    if ((nResult & REQUEST_FAIL_MASK) == REQUEST_FAIL_MASK)
    {
        return 1;
    }

    return 0;
}

static int check_cmd_done(volatile unsigned int nStatusResult)
{
    volatile unsigned int nStatus;

    if (V2FRequestReportDone(nStatusResult))
    {
        nStatus = V2FEliminateReportDoneFlag(nStatusResult);
        if (V2FRequestComplete(nStatus))
        {
            return 1;
        }
    }

    return 0;
}

static int check_cmd_success(volatile unsigned int nStatusResult)
{
    unsigned int nStatus;
    nStatus = V2FEliminateReportDoneFlag(nStatusResult);
    assert(V2FRequestComplete(nStatus));
    if (V2FRequestFail(nStatus))
    {
        return 0;
    }

    return 1;
}

void fil_nand_init(void)
{
    nand_ctrl_reg[0] = (V2FMCRegisters *)NSC_0_BASEADDR;
    nand_ctrl_reg[1] = (V2FMCRegisters *)NSC_1_BASEADDR;
    nand_ctrl_reg[2] = (V2FMCRegisters *)NSC_2_BASEADDR;
    nand_ctrl_reg[3] = (V2FMCRegisters *)NSC_3_BASEADDR;
    nand_ctrl_reg[4] = (V2FMCRegisters *)NSC_4_BASEADDR;
    nand_ctrl_reg[5] = (V2FMCRegisters *)NSC_5_BASEADDR;
    nand_ctrl_reg[6] = (V2FMCRegisters *)NSC_6_BASEADDR;
    nand_ctrl_reg[7] = (V2FMCRegisters *)NSC_7_BASEADDR;

    for (int i = 0; i < USER_CHANNELS; i++)
    {
        for (int j = 0; j < USER_WAYS; j++)
        {
            V2FResetSync(nand_ctrl_reg[i], j);
            usleep(1000);
        }
    }

    for (int i = 0; i < USER_CHANNELS; i++)
    {
        for (int j = 0; j < USER_WAYS; j++)
        {
            way_status[i][j].nCommon = OPERATION_STEP_START;
        }
    }

    nand_completion = dma_malloc(sizeof(NAND_COMPLETION), 0);
    nand_status = dma_malloc(sizeof(NAND_STATUS), 0);
    nand_error = dma_malloc(sizeof(NAND_ERROR), 0);

    for (int i = 0; i < USER_CHANNELS; i++)
    {
        for (int j = 0; j < USER_WAYS; j++)
        {
            INIT_LIST_HEAD(&request_q[i][j]);
        }
    }
}

static int process_read_request(int ch, int way, int block, int page, void *buf_main, void *buf_spare, int ecc, int sync)
{
    DEBUG_ASSERT(check_addr_valid(ch, way, block, page));

    unsigned int row_addr;
    volatile unsigned int *result_status;

    unsigned int *err_info = (unsigned int*)(nand_error->anData[ch][way]);
    unsigned int *cmpl_info = (unsigned int*)(nand_completion->anData[ch][way]);

    NAND_READ_STEP *step = &way_status[ch][way].nRead;

    switch (*step)
    {
    case NAND_READ_STEP_NONE:
        *step = NAND_READ_STEP_CHECK_READ_ISSUABLE;
        // fallthrough
    case NAND_READ_STEP_CHECK_READ_ISSUABLE:
        if (!check_issuable(ch, way))
        {
            break;
        }
        *step = NAND_READ_STEP_ISSUE_READ;
        // fallthrough
    case NAND_READ_STEP_ISSUE_READ:
        row_addr = get_row_addr(ch, way, block, page);
        V2FReadPageTriggerAsync(nand_ctrl_reg[ch], way, row_addr);
        *step = NAND_READ_STEP_WAIT_READ_DONE;
        // fallthrough
    case NAND_READ_STEP_WAIT_READ_DONE:
        if (!check_issuable(ch, way))
        {
            break;
        }
        *step = NAND_READ_STEP_ISSUE_GET_STATUS_READ;
        // fallthrough
    case NAND_READ_STEP_ISSUE_GET_STATUS_READ:
        result_status = (volatile unsigned int *)&nand_status->anData[ch][way];
        *result_status = 0;
        V2FStatusCheckAsync(nand_ctrl_reg[ch], way, (unsigned int *)result_status);

        *step = NAND_READ_STEP_WAIT_GET_STATUS_READ_DONE;
        break;
    case NAND_READ_STEP_WAIT_GET_STATUS_READ_DONE:
        if (!check_way_idle(ch, way))
        {
            break;
        }
        *step = NAND_READ_STEP_CHECK_READ_RESULT;
        // fallthrough
    case NAND_READ_STEP_CHECK_READ_RESULT:
        result_status = (volatile unsigned int *)&nand_status->anData[ch][way];
        if (check_cmd_done(*result_status))
        {
            if (!check_cmd_success(*result_status))
            {
                printf("NAND READ FAILED %d/%d/%d/%d %d\n", ch, way, block, page, way_status[ch][way].nRetry);
                way_status[ch][way].nRetry++;
                if (way_status[ch][way].nRetry > FIL_READ_RETRY)
                {
                    *step = NAND_READ_STEP_DONE;
                }
                else
                {
                    *step = NAND_READ_STEP_NONE;
                }
            }
            else
            {
                *step = NAND_READ_STEP_CHECK_TRANSFER_ISSUABLE; //fallthru?
            }
        }
        else
        {
            *step = NAND_READ_STEP_WAIT_READ_DONE;
        }
        break; // break alwayas??
    case NAND_READ_STEP_CHECK_TRANSFER_ISSUABLE:
        // no check?
        *step = NAND_READ_STEP_ISSUE_TRANSFER;
        // fallthrough
    case NAND_READ_STEP_ISSUE_TRANSFER:
        if (ecc)
        {
            row_addr = get_row_addr(ch, way, block, page);
            V2FReadPageTransferAsync(nand_ctrl_reg[ch], way, buf_main, buf_spare, err_info, cmpl_info, row_addr);
        }
        else
        {
            V2FReadPageTransferRawAsync(nand_ctrl_reg[ch], way, buf_main, cmpl_info);
        }
        *step = NAND_READ_STEP_WAIT_TRASFER_DONE;
        break;
    case NAND_READ_STEP_WAIT_TRASFER_DONE:
        if (!check_issuable(ch, way))
        {
            break;
        }
        *step = NAND_READ_STEP_CHECK_TRANSFER_RESULT;
        break;
    case NAND_READ_STEP_CHECK_TRANSFER_RESULT:
        if (V2FTransferComplete(*cmpl_info))
        {
            if (ecc && (get_transfer_result(err_info) == ERROR_INFO_FAIL))
            {
                printf("NAND READ FAILED ECC %d/%d/%d/%d %d\n", ch, way, block, page, way_status[ch][way].nRetry);
                way_status[ch][way].nRetry++;
                if (way_status[ch][way].nRetry > FIL_READ_RETRY)
                {
                    *step = NAND_READ_STEP_DONE;
                }
                else
                {
                    *step = NAND_READ_STEP_NONE;
                }
            }
            else
            {
                *step = NAND_READ_STEP_DONE;
            }
        }
        else
        {
            usleep(2);
            *step = NAND_READ_STEP_WAIT_TRASFER_DONE;
        }
        break;
    case NAND_READ_STEP_DONE:
        break;
    default:
        ASSERT(0);
    }
    if (*step == NAND_READ_STEP_DONE)
    {
        *step = NAND_READ_STEP_NONE;
        return 1;
    }
    return 0;
}

int NAND_ProcessRead(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bECCOn, int bSync)
{
	DEBUG_ASSERT(check_addr_valid(nPCh, nPWay, nBlock, nPage));

	unsigned int nRowAddr;

	NAND_READ_STEP		*pnCurStatus = &g_pstNAND->stWayStatus[nPCh][nPWay].nRead;
	DEBUG_ASSERT(*pnCurStatus < NAND_READ_STEP_COUNT);

	volatile unsigned int *pnStatusResult;

	unsigned int *panErrorInfo = (unsigned int*)(&g_pstNAND->pstNAND_Error->anData[nPCh][nPWay]);
	unsigned int *pnCompletion = (unsigned int*)(&g_pstNAND->pstNAND_Completion->anData[nPCh][nPWay]);

	static long nSleepTimeReadUS = 110;			// mandatory, LSB PGM time
	static long nSleepTimeGetStatusUS = 2;			// mandatory
	static long nSleepTimeCheckStatusUS = 2;		// mandatory
	static long nSleepTimeTransferUS = 110;		// for DMA 16KB

	NAND_RESULT nResult = NAND_RESULT_OP_RUNNING;

	do
	{
		switch (*pnCurStatus)
		{
		case NAND_READ_STEP_NONE:
			*pnCurStatus = NAND_READ_STEP_CHECK_READ_ISSUABLE;
			// fall down

		case NAND_READ_STEP_CHECK_READ_ISSUABLE:
			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				break;
			}

			*pnCurStatus = NAND_READ_STEP_ISSUE_READ;
			// Fall through

		case NAND_READ_STEP_ISSUE_READ:
			nRowAddr = _GetRowAddress(nPCh, nPWay, nBlock, nPage);

			// Issue NAND Command
			V2FReadPageTriggerAsync(chCtlReg[nPCh], nPWay, nRowAddr);
			*pnCurStatus = NAND_READ_STEP_WAIT_READ_DONE;
			break;

		case NAND_READ_STEP_WAIT_READ_DONE:
			if (bSync == TRUE)
			{
				usleep(nSleepTimeReadUS);		// mandatory sleep
			}

			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				continue;
			}

			*pnCurStatus = NAND_READ_STEP_ISSUE_GET_STATUS_READ;
			// fall through

		case NAND_READ_STEP_ISSUE_GET_STATUS_READ:
			pnStatusResult = (volatile unsigned int*)&g_pstNAND->pstNAND_Status->anData[nPCh][nPWay];
			*pnStatusResult = 0;
			V2FStatusCheckAsync(chCtlReg[nPCh], nPWay, (unsigned int*)pnStatusResult);

			*pnCurStatus = NAND_READ_STEP_WAIT_GET_STATUS_READ_DONE;
			break;

		case NAND_READ_STEP_WAIT_GET_STATUS_READ_DONE:
			if (bSync == TRUE)
			{
				usleep(nSleepTimeGetStatusUS);		// mandatory sleep
			}

			if (_IsWayIdle(nPCh, nPWay) == FALSE)
			{
				continue;
			}

			*pnCurStatus = NAND_READ_STEP_CHECK_READ_RESULT;
			// fall through

		case NAND_READ_STEP_CHECK_READ_RESULT:
			pnStatusResult = (volatile unsigned int*)&g_pstNAND->pstNAND_Status->anData[nPCh][nPWay];
			if (_IsCmdDone(*pnStatusResult))
			{
				if (_IsCmdSuccess(*pnStatusResult) == FALSE)
				{
					// Fail
					PRINTF("NAND Read Fail - Ch/Way/Block/Page:%d/%d/%d/%d, Try:%d\r\n", nPCh, nPWay, nBlock, nPage, g_pstNAND->stWayStatus[nPCh][nPWay].nRetry);

					g_pstNAND->stWayStatus[nPCh][nPWay].nRetry++;
					if (g_pstNAND->stWayStatus[nPCh][nPWay].nRetry > FIL_READ_RETRY)
					{
						*pnCurStatus = NAND_READ_STEP_DONE;
					}
					else
					{
						*pnCurStatus = NAND_READ_STEP_NONE;
					}
				}
				else
				{
					// Success
					*pnCurStatus = NAND_READ_STEP_CHECK_TRANSFER_ISSUABLE;
				}
			}
			else
			{
				//printf("NAND Read tR Status - 0x%X\r\n", *pnStatusResult);
				*pnCurStatus = NAND_READ_STEP_WAIT_READ_DONE;
			}

			break;

		case NAND_READ_STEP_CHECK_TRANSFER_ISSUABLE:
			*pnCurStatus = NAND_READ_STEP_ISSUE_TRANSFER;
			//break;		// fall through

		case NAND_READ_STEP_ISSUE_TRANSFER:
			if (bSync == TRUE)
			{
				usleep(nSleepTimeGetStatusUS);		// mandatory sleep
			}

			if (bECCOn == TRUE)
			{
				nRowAddr = _GetRowAddress(nPCh, nPWay, nBlock, nPage);
				V2FReadPageTransferAsync(chCtlReg[nPCh], nPWay, pMainBuf, pSpareBuf, panErrorInfo, pnCompletion, nRowAddr);
			}
			else
			{
				V2FReadPageTransferRawAsync(chCtlReg[nPCh], nPWay, pMainBuf, pnCompletion);
			}

			*pnCurStatus = NAND_READ_STEP_WAIT_TRASFER_DONE;
			break;

		case NAND_READ_STEP_WAIT_TRASFER_DONE:
			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				break;;
			}

			*pnCurStatus = NAND_READ_STEP_CHECK_TRANSFER_RESULT;
			break;

		case NAND_READ_STEP_CHECK_TRANSFER_RESULT:
			if (bSync == TRUE)
			{
				usleep(nSleepTimeTransferUS);		// mandatory sleep
			}

			if (V2FTransferComplete(*pnCompletion))
			{
				if ((bECCOn == TRUE) && (_GetTransferResult(panErrorInfo) == ERROR_INFO_FAIL))
				{
					PRINTF("NAND Read Fail UECC - Ch/Way/Block/Page:%d/%d/%d/%d, Error0/Error1:0x%X/0x%X, Try:%d\r\n", 
									nPCh, nPWay, nBlock, nPage, panErrorInfo[0], panErrorInfo[1], g_pstNAND->stWayStatus[nPCh][nPWay].nRetry);
					g_pstNAND->stWayStatus[nPCh][nPWay].nRetry++;
					if (g_pstNAND->stWayStatus[nPCh][nPWay].nRetry > FIL_READ_RETRY)
					{
						*pnCurStatus = NAND_READ_STEP_DONE;
					}
					else
					{
						*pnCurStatus = NAND_READ_STEP_NONE;
					}
				}
				else
				{
					*pnCurStatus = NAND_READ_STEP_DONE;
				}
			}
			else
			{
				//printf("NAND Read Transfer Status - Completion: 0x%X\r\n", *pnCompletion);
				usleep(nSleepTimeCheckStatusUS);	// mandatory

				*pnCurStatus = NAND_READ_STEP_WAIT_TRASFER_DONE;
			}

			break;

		case NAND_READ_STEP_DONE:
			// done, do nothing just break
			break;

		default:
			assert(0);
		}

		if (*pnCurStatus == NAND_READ_STEP_DONE)
		{

			*pnCurStatus = NAND_READ_STEP_NONE;
			nResult = NAND_RESULT_DONE;
			break;
		}

	} while (bSync == TRUE);

	return nResult;
}

static int process_program_request(int ch, int way, int block, int page, void* buf_main, void* buf_spare, int bSync)
{
    DEBUG_ASSERT(check_addr_valid(ch, way, block, page));

    unsigned int row_addr;

    volatile unsigned int *result_status;

    static long nSleepTimeProgramUS = 110;          // mandatory, LSB PGM time
    static long nSleepTimeGetStatusUS = 1;          // mandatory
    static long nSleepTimeCheckStatusUS = 1;        // mandatory

    NAND_PROGRAM_STEP *step = &way_status[ch][way].nProgram;
    DEBUG_ASSERT(*step < NAND_PGM_STEP_COUNT);

    do
    {
        switch (*step)
        {
        case NAND_PGM_STEP_NONE:
            *step = NAND_PGM_STEP_CHECK_PROGRAM_ISSUABLE;
            // fall down
        case NAND_PGM_STEP_CHECK_PROGRAM_ISSUABLE:
            if (!check_issuable(ch, way))
            {
                break;
            }

            *step = NAND_PGM_STEP_ISSUE_PROGRAM;
            // Fall through
        case NAND_PGM_STEP_ISSUE_PROGRAM:
            row_addr = get_row_addr(ch, way, block, page);

            // Issue NAND Command
            V2FProgramPageAsync(nand_ctrl_reg[ch], way, row_addr, buf_main, buf_spare);
            *step = NAND_PGM_STEP_WAIT_PROGRAM_DONE;
            break;

        case NAND_PGM_STEP_WAIT_PROGRAM_DONE:
            if (bSync)
            {
                usleep(nSleepTimeProgramUS);        // mandatory sleep
            }

            if (!check_issuable(ch, way))
            {
                break;
            }

            *step = NAND_PGM_STEP_ISSUE_GET_STATUS;
            // fall through

        case NAND_PGM_STEP_ISSUE_GET_STATUS:
            result_status = (volatile unsigned int*)&nand_status->anData[ch][way];
            *result_status = 0;
            V2FStatusCheckAsync(nand_ctrl_reg[ch], way, (unsigned int*)result_status);

            *step = NAND_PGM_STEP_WAIT_GET_STATUS_DONE;
            break;

        case NAND_PGM_STEP_WAIT_GET_STATUS_DONE:
            if (bSync)
            {
                usleep(nSleepTimeGetStatusUS);        // mandatory sleep
            }

            if (!check_way_idle(ch, way))
            {
                break;
            }

            *step = NAND_PGM_STEP_CHECK_STATUS;
            // fall through

        case NAND_PGM_STEP_CHECK_STATUS:
            result_status = (volatile unsigned int*)&nand_status->anData[ch][way];
            if (check_cmd_done(*result_status))
            {
                if (!check_cmd_success(*result_status))
                {
                    // Fail
                    printf("NAND Program Fail - Ch/Way/Block/Page:%d/%d/%d/%d\r\n", ch, way, block, page);
                }
                else
                {
                    // Success
                }

                *step = NAND_PGM_STEP_DONE;
            }
            else
            {
                //printf("NAND Program Status - 0x%X\r\n", *result_status);
                if (!check_request_complete(*result_status))
                {
                    *step = NAND_PGM_STEP_WAIT_PROGRAM_DONE;
                }

                if (bSync)
                {
                    usleep(nSleepTimeCheckStatusUS);    // mandatory
                }
            }
            break;

        default:
            assert(0);
        }

        if (*step == NAND_PGM_STEP_DONE)
        {
            // Program Done
            *step = NAND_PGM_STEP_NONE;
            return 1;
        }
    } while (bSync);

    return 0;
}

static int process_erase_request(int ch, int way, int block, int bSync)
{
    DEBUG_ASSERT(check_addr_valid(ch, way, block, 0));
    unsigned int row_addr;

    NAND_ERASE_STEP	*step = &way_status[ch][way].nErase;
    DEBUG_ASSERT(*step < NAND_ERASE_STEP_COUNT);

    int bIdle;
    volatile unsigned int *result_status;

    static long nSleepTimeEraseUS = 7000;			// mandatory
    static long nSleepTimeGetStatusUS = 1;			// mandatory
    static long nSleepTimeCheckStatusUS = 1;		// mandatory

    do
    {
        printf("step: %d\n", *step);
        switch (*step)
        {
        case NAND_ERASE_STEP_NONE:
            *step = NAND_ERASE_STEP_CHECK_ERASE_ISSUABLE;
            // fall down

        case NAND_ERASE_STEP_CHECK_ERASE_ISSUABLE:
            if (!check_issuable(ch, way))
            {
                if (bSync)
                {
                    continue;
                }

                break;		// try at next time
            }

            *step = NAND_ERASE_STEP_ISSUE_ERASE;
            // Fall through

        case NAND_ERASE_STEP_ISSUE_ERASE:
            row_addr = get_row_addr(ch, way, block, 0 /*Don't Care */);

            // Issue NAND Command
            V2FEraseBlockAsync(nand_ctrl_reg[ch], way, row_addr);
            *step = NAND_ERASE_STEP_WAIT_ERASE_DONE;
            break;

            //usleep(nSleepTimeUS);		// no need to sleep
        case NAND_ERASE_STEP_WAIT_ERASE_DONE:
            if (!check_issuable(ch, way))
            {
                break;
            }

            *step = NAND_ERASE_STEP_ISSUE_GET_STATUS;
            // fall through

            if (bSync)
            {
                usleep(nSleepTimeEraseUS);		// mandatory sleep
            }
        case NAND_ERASE_STEP_ISSUE_GET_STATUS:
            result_status = (volatile unsigned int*)&nand_status->anData[ch][way];
            *result_status = 0;
            V2FStatusCheckAsync(nand_ctrl_reg[ch], way, (unsigned int*)result_status);

            *step = NAND_ERASE_STEP_WAIT_GET_STATUS_DONE;
            break;

        case NAND_ERASE_STEP_WAIT_GET_STATUS_DONE:
            if (bSync)
            {
                usleep(nSleepTimeGetStatusUS);		// mandatory sleep
            }

            bIdle = check_way_idle(ch, way);
            if (!bIdle)
            {
                break;
            }

            *step = NAND_ERASE_STEP_CHECK_STATUS;
            // fall through

        case NAND_ERASE_STEP_CHECK_STATUS:
            result_status = (volatile unsigned int*)&nand_status->anData[ch][way];
            if (check_cmd_done(*result_status))
            {
                if (!check_cmd_success(*result_status))
                {
                    // Fail
                    printf("NAND Erase Fail - Ch/Way/Block:%d/%d/%d\r\n", ch, way, block);
                }
                else
                {
                    // Success
                }

                *step = NAND_ERASE_STEP_DONE;
            }
            else
            {
                //printf("NAND Erase Status - 0x%X\r\n", *result_status);

                if (!check_request_complete(*result_status))
                {
                    *step = NAND_ERASE_STEP_WAIT_ERASE_DONE;
                }

                if (bSync)
                {
                    usleep(nSleepTimeCheckStatusUS);	// mandatory
                }
            }

            break;

        default:
            assert(0);
            break;
        }

        if (*step == NAND_ERASE_STEP_DONE)
        {
            // Erase Done
            *step = NAND_ERASE_STEP_NONE;
            return 1;
        }
    } while (bSync);

    return 0;
}

void fil_nand_run(void)
{
    for (int i = 0; i < USER_CHANNELS; i++)
    {
        for (int j = 0; j < USER_WAYS; j++)
        {
            if (list_empty(&request_q[i][j])) // 아니 이런거 까지 괄호 쓰는거 오바임
            {
                continue;
            }

            dindent(6);
            dprint("[process_request_queue] ch %d, way %d\n", i, j);

            struct nand_req *req = list_first_entry(&request_q[i][j], struct nand_req, list);


            int result = 0;

            switch (req->type)
            {
                case NAND_TYPE_READ:
                    result = process_read_request(i, j, req->addr.block, req->addr.page, req->buf_main, req->buf_spare, 1, 0);
                    break;
                case NAND_TYPE_PROGRAM:
                    result = process_program_request(i, j, req->addr.block, req->addr.page, req->buf_main, req->buf_spare, 0);
                    break;
                case NAND_TYPE_ERASE:
                    result = process_erase_request(i, j, req->addr.block, 0);
                    break;
                default:
                    ASSERT(0);
            }

            if (result == 1)
            {
                struct cmd *cmd = req->cmd;

                if (req->type != NAND_TYPE_ERASE)
                {
                    cmd->ndone++;

                    dindent(6);
                    dprint("tag: %d lba %u cnt %u\n", cmd->tag, cmd->lba, cmd->nblock);
                    dindent(6);
                    dprint("%u/%u\n", cmd->ndone, cmd->nblock);
                    if (cmd->ndone >= cmd->nblock)
                    {
                        dindent(6);
                        if (cmd->type == CMD_TYPE_RD)
                        {
                            q_push_tail(&read_dma_wait_q, cmd);
                            dprint("rd cmd done\n");
                        }
                        else
                        {
                            q_push_tail(&done_q, cmd);
                            dprint("wr cmd done\n");
                        }
                    }
                }

                list_del(&req->list);
                free(req); // TODO use req pool
            }
        }
    }
}

void fil_add_req(struct cmd *cmd, int type, void *buf_main, void *buf_spare, nand_addr_t addr)
{
    dindent(6);
    dprint("[fil_add_req]\n");

    struct nand_req *req = malloc(sizeof(struct nand_req)); //TODO use req pool
    req->type = type;
    req->addr = addr;
    req->cmd = cmd;
    req->buf_main = buf_main;
    req->buf_spare = buf_spare;

    list_add_tail(&req->list, &request_q[addr.ch][addr.way]);

    dindent(6);
    dprint("added to request_q\n");
}
