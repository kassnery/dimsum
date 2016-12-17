CXXFLAGS=-O2 -DNDEBUG
CXX=g++

PCAP=-L../lib/winpcap/Lib -llibwpcap -I../lib/winpcap/Include
OBJECTS=rand48.o qdigest.o prng.o lossycount.o gk.o frequent.o countmin.o cgt.o ccfc.o 


all: $(OBJECTS)
	$(CXX) $(CXXFLAGS) hh.cc $(OBJECTS) -o Release/hh-zipf
	$(CXX) $(CXXFLAGS) hh.cc $(OBJECTS) -o Release/hh-pcap -DPCAP $(PCAP)

$(OBJECTS): rand48.h qdigest.h prng.h lossycount.h gk4.h frequent.h countmin.h cgt.h ccfc.h
	$(CXX) $(CXXFLAGS) -c $*.cc

clean:
	rm -rf *.o Release/hh-zipf Release/hh-zipf.exe Release/hh-pcap Release/hh-pcap.exe
