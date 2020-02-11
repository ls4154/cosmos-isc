BASESRC = $(wildcard common/*.c) $(wildcard hil/*.c) $(wildcard hil/nvme/*.c) $(wildcard fil/*.c) $(wildcard target/*.c)
FLAGS = -I. -pthread -Wall

ramftl:
	gcc $(BASESRC) $(wildcard ftl/ramftl/*.c) $(FLAGS)
cross:
	arm-linux-gnueabi-gcc $(BASESRC) $(wildcard ftl/ramftl/*.c) $(FLAGS)
clean:
	rm a.out
