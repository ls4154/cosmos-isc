CXXFLAGS = -std=c++14
LDFLAGS = -liscssd

ifeq ($(PREFIX),)
	PREFIX := /usr
endif

all: wordcount wordcount_single

wordcount: wordcount_ssd.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

wordcount_single: wordcount_ssd_single.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

install: wordcount wordcount_single
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 4755 wordcount $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 4755 wordcount_single $(DESTDIR)$(PREFIX)/bin/

clean:
	rm wordcount wordcount_single
