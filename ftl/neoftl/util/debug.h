#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <assert.h>
#include <sys/time.h>

#define __ASSERT 1

#if __ASSERT
#define ASSERT assert
#else
#define ASSERT(X)
#endif

#ifdef _DEBUG
#define DEBUG_ASSERT ASSERT
#else
#define DEBUG_ASSERT(ignore)
#endif

#define FATAL \
	do { \
		fprintf(stderr, "\nError in %s: Line %d\n", __FILE__, __LINE__); \
		exit(1); \
	} while (0)

#ifdef NO_DEBUG
#define dprint(x...) do {} while (0)
#else
#define dprint(x...) printf(x);
#endif
#define dindent(x) dprint("%" #x "0s", "")

#define cprint(cond, ...) do { if (cond) printf(__VA_ARGS__); } while (0)

#define TV_DEF(name) \
	struct timeval name

#define TV_GET(name) \
	gettimeofday(&(name), NULL)

#define TV_ABS(prefix, tv) \
	fprintf(stderr, #prefix " %ld.%ld\n", (tv).tv_sec, (tv).tv_usec)

#define TV_DIF(prefix, tv_s, tv_e) \
	fprintf(stderr, #prefix " %ld.%06ld\n", \
			(tv_e).tv_sec - (tv_s).tv_sec - ((tv_e).tv_usec < (tv_s).tv_usec), \
			((tv_e).tv_usec - (tv_s).tv_usec + 1000000) % 1000000)

#endif /* DEBUG_H */
