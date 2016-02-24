CXX := g++
CXXFLAGS := -DHAVE_CONFIG_H -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -g -O2 
INCLUDES :=	-I. -I..
LIBS     := -ljemalloc -lm -pthread -lprotobuf -lpthread -ljemalloc 

all: test.o

%.o: %.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $(input) -o $(output)
