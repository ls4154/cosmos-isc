BASESRC = $(wildcard common/*.c) $(wildcard hil/*.c) $(wildcard target/*.c)
FLAGS = -I. -Wall
MT_FLAGS = -pthread -D FTL_MULTI_THREAD
CC = gcc
ifneq ($(shell uname -p),armv7l)
	CC = arm-linux-gnueabi-gcc
endif

nand:
	$(CC) $(BASESRC) $(wildcard ftl/simple/*.c) $(wildcard fil/nand/*.c) $(FLAGS)
nand_mt:
	$(CC) $(BASESRC) $(wildcard ftl/simple/*.c) $(wildcard fil/nand/*.c) $(FLAGS) $(MT_FLAGS)
ram:
	$(CC) $(BASESRC) $(wildcard ftl/passthru/*.c) $(wildcard fil/ram/*.c) $(FLAGS)
ram_mt:
	$(CC) $(BASESRC) $(wildcard ftl/passthru/*.c) $(wildcard fil/ram/*.c) $(FLAGS) $(MT_FLAGS)
clean:
	rm a.out
