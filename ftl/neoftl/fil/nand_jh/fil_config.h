#ifndef __FIL_CONFIG_H__
#define __FIL_CONFIG_H__

#include "common/cosmos_plus_system.h"

//#define FTL_REQUEST_COUNT		(USER_CHANNELS * USER_WAYS * 2)		// #CH * #wAY * double Buffering

#define FIL_READ_RETRY			(5)			// re-try count when UECC

// DEBUG CONFIG
#define NAND_DEBUG				(1)

#endif	// end of #ifdef __FIL_CONFIG_H__




