CXXFLAGS = -O2 -std=c++14
LDFLAGS = -liscssd

ifeq ($(PREFIX),)
	PREFIX := /usr
endif

wordcount: wordcount_ssd.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

install: wordcount
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 4755 wordcount $(DESTDIR)$(PREFIX)/bin/

clean:
	rm wordcount
