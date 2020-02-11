#ifndef __HIL_CONFIG_H__
#define __HIL_CONFIG_H__

#define HOST_REQUEST_COUNT		(256)
#define HOST_REQUEST_BUF_COUNT	(HOST_REQUEST_COUNT * 8)

#define HIL_DEBUG		(0)

#if (HIL_DEBUG == 0)
	#define HIL_DEBUG_PRINTF(...)		((void)0)
#else
	#define HIL_DEBUG_PRINTF		PRINTF
#endif

#endif
