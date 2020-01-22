CFLAGS = -O2 -std=c11
LDFLAGS = -liscssd

all: rd wr

rd: lba_reader.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

wr: lba_writer.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm rd wr
