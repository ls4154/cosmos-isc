CXXFLAGS = -O2 -std=c++14
LDFLAGS = -liscssd

wordcount: wordcount_ssd.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm wordcount
