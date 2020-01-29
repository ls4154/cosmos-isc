CFLAGS = -std=c11
LDFLAGS = -liscssd

ifeq ($(PREFIX),)
	PREFIX := /usr
endif

all: rd wr

rd: lba_reader.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

wr: lba_writer.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install: rd wr
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 4755 rd $(DESTDIR)$(PREFIX)/bin/
	install -m 4755 wr $(DESTDIR)$(PREFIX)/bin/

clean:
	rm rd wr
