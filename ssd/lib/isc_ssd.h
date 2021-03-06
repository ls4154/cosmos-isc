#ifndef ISC_SSD_H
#define ISC_SSD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define dprint(...) do { fprintf(stderr, __VA_ARGS__); } while (0)
#else
#define dprint(...) do { } while (0)
#endif

#define SECTSIZE 4096

enum {
	ISC_OP_RD = 1,
	ISC_OP_WR = 2
};

enum {
	ISC_ST_IDLE = 0,
	ISC_ST_REQ = 1,
	ISC_ST_WIP = 2,
	ISC_ST_DONE = 3
};

typedef struct shared {
	int opcode;
	volatile int status;
	int lba;
	int sector_cnt;
	union {
		char payload[1024][4][SECTSIZE];
		char payload_write[16 * 1024 * 1024];
	};
	int payload_ofs[1024];
} Shared;

Shared *isc_shared;

static inline int to_sect(int bytes)
{
	return (bytes + SECTSIZE - 1) / SECTSIZE;
}

static inline int isc_check_done(int pp)
{
	if (isc_shared[pp].status == ISC_ST_DONE) {
		isc_shared[pp].status = ISC_ST_IDLE;
		return 1;
	}
	return 0;
}

static inline char *isc_payload(int pp, int idx)
{
	return isc_shared[pp].payload[idx][isc_shared[pp].payload_ofs[idx]];
}

int isc_init_shared(void);

int isc_read_request(int pp, int lba, int bytes);

int isc_write_request(int pp, int lba, int bytes);

#include <stdio.h>
#include <sys/time.h>

#define TV_DEF(name) \
	struct timeval name

#define TV_GET(name) \
	gettimeofday(&(name), NULL)

#define TV_ABS(prefix, tv) \
	fprintf(stderr, #prefix " %ld.%ld\n", (tv).tv_sec, (tv).tv_usec)

#define TV_DIF(prefix, tv_s, tv_e) \
	fprintf(stderr, #prefix " %ld.%ld\n", \
			(tv_e).tv_sec - (tv_s).tv_sec - ((tv_e).tv_usec < (tv_s).tv_usec ), \
			((tv_e).tv_usec - (tv_s).tv_usec + 1000000) % 1000000)

#ifdef __cplusplus
}
#endif

#endif
