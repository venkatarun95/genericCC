MEMORY_STYLE := ./protobufs-default

CXX := g++
CXXFLAGS := -DHAVE_CONFIG_H -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -g -O2 
INCLUDES :=	-I./protobufs-default -I./udt -I./tests -I.
LIBS     := -ljemalloc -lm -pthread -lprotobuf -lpthread -ljemalloc -ludt 

OBJECTS := random.o memory.o memoryrange.o rat.o whisker.o						\
whiskertree.o udp-socket.o traffic-generator.o remycc.o markoviancc.o	\
utilities.o

all: sender receiver prober test

.PHONY: all

sender: $(OBJECTS) sender.o $(MEMORY_STYLE)/libremyprotos.a
	$(CXX) $(inputs) -o $(output) $(LIBS)

prober: prober.o udp-socket.o
	$(CXX) $(inputs) -o $(output) $(LIBS)

receiver: receiver.o udp-socket.o
	$(CXX) $(inputs) -o $(output) $(LIBS)

test: tests/test.o send-window.o
	$(CXX) $(inputs) -o $(output) $(LIBS)

%.o: %.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $(input) -o $(output)

pcc-tcp.o: pcc-tcp.cc
	 g++ -DHAVE_CONFIG_H -I. -I./udt -std=c++11 -pthread         -fno-default-inline -g -O2 -MT pcc-tcp.o -MD -MP -c -o pcc-tcp.o pcc-tcp.cc 
