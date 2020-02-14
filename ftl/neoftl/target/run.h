#ifndef RUN_H

#define RUN_H

typedef void (*run_func_t)(void);

#define LIST_RUN(name, x...) \
	static run_func_t name[] = { x, NULL };

#ifdef FTL_MULTI_THREAD

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

void *th_func(void *args)
{
	int err;
	run_func_t f = args;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGUSR2);

	err = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (err) {
		perror("sigmask");
		exit(1);
	}

	while (1)
		f();
	return NULL;
}

#define RUN(list) \
	do { \
		pthread_t tid; \
		for (run_func_t *f = list + 1; *f; f++) { \
			int err; \
			err = pthread_create(&tid, NULL, th_func, *f); \
			if (err) { \
				perror("pthread create"); \
				exit(1); \
			} \
			printf("thread start\n"); \
		} \
		printf("main thread start\n"); \
		while (1) \
			(*list)(); \
	} while (0)

#else

#define RUN(list) \
	do { \
		while (1) \
			for (run_func_t *f = list; *f; f++) \
				(*f)(); \
	} while (0)

#endif

#define RUN_ONCE(list) \
	do { \
		for (run_func_t *f = list; *f; f++) \
			(*f)(); \
	} while (0)

#endif
