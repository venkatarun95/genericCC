pMEMORY_STYLE := ./protobufs-default

CXX := g++
CXXFLAGS := -DHAVE_CONFIG_H -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -g -O2 -fPIC
INCLUDES :=	-I./protobufs-default -I./udt

LIBS     := -ljemalloc -lm -pthread -lprotobuf -lpthread -ljemalloc
#$(MEMORY_STYLE)/libremyprotos.a
OBJECTS  := random.o memory.o memoryrange.o rat.o whisker.o whiskertree.o udp-socket.o traffic-generator.o remycc.o markoviancc.o estimators.o rtt-window.o #protobufs-default/dna.pb.o

all: sender receiver pygenericcc.so

.PHONY: all

protobufs-default/dna.pb.cc: protobufs-default/dna.proto
	protoc --cpp_out=. protobufs-default/dna.proto

protobufs-default/dna.pb.o: protobufs-default/dna.pb.cc
	$(CXX) -I.. -I. -O2 -fPIC -c protobufs-default/dna.pb.cc -o protobufs-default/dna.pb.o

sender: $(OBJECTS) sender.o protobufs-default/dna.pb.o # $(MEMORY_STYLE)/libremyprotos.a
	$(CXX) $(inputs) -o $(output) $(LIBS)

prober: prober.o udp-socket.o
	$(CXX) $(inputs) -o $(output) $(LIBS)

receiver: receiver.o udp-socket.o
	$(CXX) $(inputs) -o $(output) $(LIBS)

python-wrapper.o: python-wrapper.cc
	g++ -I/usr/include/python2.7 $(INCLUDES) -fPIC $(CXXFLAGS) -c python-wrapper.cc -o python-wrapper.o

%.o: %.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $(input) -o $(output)

pygenericcc.so:
  $(CXX) -shared -Wl,--export-dynamic -Wl,--no-undefined python-wrapper.o $(OBJECTS) protobufs-default/dna.pb.o -o pygenericcc.so -lpython2.7 -lboost_python $(LIBS)

pcc-tcp.o: pcc-tcp.cc
	 g++ -DHAVE_CONFIG_H -I. -I./udt -std=c++11 -pthread         -fno-default-inline -g -O2 -MT pcc-tcp.o -MD -MP -c -o pcc-tcp.o pcc-tcp.cc 
