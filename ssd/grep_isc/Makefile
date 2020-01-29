CXXFLAGS = -std=c++14
LDFLAGS = -liscssd

ifeq ($(PREFIX),)
	PREFIX := /usr
endif

all: grep grep_single

grep: grep_ssd.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

grep_single: grep_ssd_single.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

install: grep grep_single
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 4755 grep $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 4755 grep_single $(DESTDIR)$(PREFIX)/bin/

clean:
	rm grep grep_single
