CFLAGS = -std=c11 -fPIC

ifeq ($(PREFIX),)
	PREFIX := /usr
endif

libiscssd.so: isc_ssd.o
	gcc -shared -o $@ $^

isc_ssd.o: isc_ssd.c

install: libiscssd.so isc_ssd.h
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 755 libiscssd.so $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 755 isc_ssd.h $(DESTDIR)$(PREFIX)/include/

clean:
	rm *.o *.so
