#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <assert.h>

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
	} while(0)

#endif /* DEBUG_H */
