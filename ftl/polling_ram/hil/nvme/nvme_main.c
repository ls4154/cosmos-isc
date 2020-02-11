#include <unistd.h>

#include "common/debug.h"
#include "common/io_access.h"

#include "nvme.h"
#include "host_lld.h"
#include "nvme_main.h"
#include "nvme_admin_cmd.h"
#include "nvme_io_cmd.h"

#include "nvme_main.h"

#include "common/cosmos_plus_system.h"

volatile NVME_CONTEXT g_nvmeTask;

void nvme_run(void)
{
	if (g_nvmeTask.status == NVME_TASK_WAIT_CC_EN) {
		unsigned int ccEn;
		ccEn = check_nvme_cc_en();
		if (ccEn == 1) {
			set_nvme_admin_queue(1, 1, 1);
			set_nvme_csts_rdy(1);
			g_nvmeTask.status = NVME_TASK_RUNNING;
			printf("\r\nNVMe ready!!!\r\n");
		}
	}
	else if (g_nvmeTask.status == NVME_TASK_RUNNING) {
		NVME_COMMAND nvmeCmd;
		unsigned int cmdValid;

		cmdValid = get_nvme_cmd(&nvmeCmd.qID, &nvmeCmd.cmdSlotTag, &nvmeCmd.cmdSeqNum, nvmeCmd.cmdDword);

		if (cmdValid == 1) {
			if (nvmeCmd.qID == 0)
				handle_nvme_admin_cmd(&nvmeCmd);
			else
				handle_nvme_io_cmd(&nvmeCmd);
		}
	}
	else if (g_nvmeTask.status == NVME_TASK_SHUTDOWN) {
		NVME_STATUS_REG nvmeReg;
		nvmeReg.dword = IO_READ32(NVME_STATUS_REG_ADDR);
		if (nvmeReg.ccShn != 0) {
			unsigned int qID;
			set_nvme_csts_shst(1);

			for (qID = 0; qID < 8; qID++) {
				set_io_cq(qID, 0, 0, 0, 0, 0, 0);
				set_io_sq(qID, 0, 0, 0, 0, 0);
			}

			set_nvme_admin_queue(0, 0, 0);
			g_nvmeTask.cacheEn = 0;
			set_nvme_csts_shst(2);
			g_nvmeTask.status = NVME_TASK_WAIT_RESET;

			printf("\r\nNVMe shutdown!!!\r\n");
		}
	}
	else if (g_nvmeTask.status == NVME_TASK_WAIT_RESET) {
		unsigned int ccEn;
		ccEn = check_nvme_cc_en();
		if (ccEn == 0) {
			g_nvmeTask.cacheEn = 0;
			set_nvme_csts_shst(0);
			set_nvme_csts_rdy(0);
			g_nvmeTask.status = NVME_TASK_IDLE;
			printf("\r\nNVMe disable!!!\r\n");
		}
	}
	else if (g_nvmeTask.status == NVME_TASK_RESET) {
		unsigned int qID;
		for (qID = 0; qID < 8; qID++) {
			set_io_cq(qID, 0, 0, 0, 0, 0, 0);
			set_io_sq(qID, 0, 0, 0, 0, 0);
		}
		g_nvmeTask.cacheEn = 0;
		set_nvme_admin_queue(0, 0, 0);
		set_nvme_csts_shst(0);
		set_nvme_csts_rdy(0);
		g_nvmeTask.status = NVME_TASK_IDLE;

		printf("\r\nNVMe reset!!!\r\n");
	}
}


