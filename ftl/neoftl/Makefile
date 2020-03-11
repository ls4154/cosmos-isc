BASESRC = $(wildcard common/*.c) $(wildcard hil/*.c) $(wildcard target/*.c)
FLAGS = -I. -Wall

nand:
	gcc $(BASESRC) $(wildcard ftl/simple/*.c) $(wildcard fil/nand/*.c) $(FLAGS)
nand_cross:
	arm-linux-gnueabi-gcc $(BASESRC) $(wildcard ftl/simple/*.c) $(wildcard fil/nand/*.c) $(FLAGS)
single:
	gcc $(BASESRC) $(wildcard ftl/passthru/*.c) $(wildcard fil/ram/*.c) $(FLAGS)
thread:
	gcc $(BASESRC) $(wildcard ftl/passthru/*.c) $(wildcard fil/ram/*.c) $(FLAGS) -pthread -D FTL_MULTI_THREAD
cross:
	arm-linux-gnueabi-gcc $(BASESRC) $(wildcard ftl/passthru/*.c) $(wildcard fil/ram/*.c) $(FLAGS)
clean:
	rm a.out
