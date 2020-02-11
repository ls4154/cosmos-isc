BASESRC = $(wildcard common/*.c) $(wildcard hil/*.c) $(wildcard target/*.c)
FLAGS = -I. -pthread -Wall

single:
	gcc $(BASESRC) $(wildcard ftl/passthru/*.c) $(wildcard fil/ram/*.c) $(FLAGS)
thread:
	gcc $(BASESRC) $(wildcard ftl/passthru/*.c) $(wildcard fil/ram/*.c) $(FLAGS) -D FTL_MULTI_THREAD
cross:
	arm-linux-gnueabi-gcc $(BASESRC) $(wildcard ftl/passthru/*.c) $(wildcard fil/ram/*.c) $(FLAGS)
clean:
	rm a.out
