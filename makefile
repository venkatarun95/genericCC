MEMORY_STYLE := ./protobufs-default

CXX := g++
CXXFLAGS := -DHAVE_CONFIG_H -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -g -O2 
INCLUDES := -I.	-I./protobufs-default -I./udt

LIBS     := -ljemalloc -lm -pthread -lprotobuf -lpthread -ljemalloc -ludt $(MEMORY_STYLE)/libremyprotos.a
OBJECTS  := random.o memory.o memoryrange.o rat.o whisker.o whiskertree.o udp-socket.o traffic-generator.o remycc.o pcc-tcp.o

all: sender receiver prober

.PHONY: all

sender: $(OBJECTS) sender.o
	$(CXX) $(inputs) -o $(output) $(LIBS)

prober: prober.o udp-socket.o
	$(CXX) $(inputs) -o $(output) $(LIBS)

receiver: receiver.o udp-socket.o
	$(CXX) $(inputs) -o $(output) $(LIBS)

%.o: %.cc
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $(input) -o $(output)

pcc-tcp.o: pcc-tcp.cc
	echo Warning: ignoring pcc-tcp.cc

###############################################33

# # Makefile

# CC   = g++
# LD   = g++

# # Toolchain arguments.
# CXXFLAGS  = -DHAVE_CONFIG_H -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -g -O2 
# LDFLAGS   = 

# # Project sources.
# C_SOURCE_FILES = random.cc memory.cc memoryrange.cc rat.cc whisker.cc whiskertree.cc socket.cc generator.cc remycc.cc
# C_OBJECT_FILES = $(patsubst %.c,%.o,$(C_SOURCE_FILES))

# # The dependency file names.
# DEPS = $(C_OBJECT_FILES:.o=.d)

# all: sender receiver prober

# clean:
# 	$(RM) $(C_OBJECT_FILES) $(DEPS) $(BIN)

# rebuild: clean all

# sender: $(C_OBJECT_FILES)
# 	$(LD) $(C_OBJECT_FILES) $(LDFLAGS) -o $@

# receiver: $(C_OBJECT_FILES)
# 	$(LD) $(C_OBJECT_FILES) $(LDFLAGS) -o $@

# prober: $(C_OBJECT_FILES)
# 	$(LD) $(C_OBJECT_FILES) $(LDFLAGS) -o $@

# %.o: %.c
# 	$(CC) -c -MMD -MP $< -o $@ $(CFLAGS)

# # Let make read the dependency files and handle them.
# -include $(DEPS)

###############################################33

# CHANGING_OPTIONS	= -I./protobufs-default -I./udt
# #STD_OPTIONS			= -DHAVE_CONFIG_H -I. -std=c++11 



# DEPFILE	= .depends
# DEPTOKEN	= '\# MAKEDEPENDS'
# DEPFLAGS	= -Y -f $(DEPFILE) -s $(DEPTOKEN) -p $(OUTDIR)/


# SRCS	 = random.cc memory.cc memoryrange.cc rat.cc whisker.cc whiskertree.cc socket.cc generator.cc remycc.cc tcp.cc
# OBJS	 = $(SRCS:.cc=.o)
# OBJS_O	= $(foreach obj, $(OBJS), $(OUTDIR)/$(obj) )


# depend:
# rm -f $(DEPFILE)
# make $(DEPFILE)


# $(DEPFILE):
# @echo $(DEPTOKEN) > $(DEPFILE)
# g++ -E -MM *.cc -I. -std=c++11 $(CHANGING_OPTIONS) > $(DEPFILE)

# # put this file in the last line of your Makefile
# sinclude $(DEPFILE)

###############################################33

# 'make depend' uses makedepend to automatically generate dependencies 
#           (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files


# # define the C compiler to use
# CC = gcc

# # define any compile-time flags
# MEMORY_STYLE = ./protobufs-default
# CFLAGS = -DHAVE_CONFIG_H -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -g -O2 

# # define any directories containing header files other than /usr/include
# #
# INCLUDES = -I. -I$(MEMORY_STYLE) -I./udt

# # define library paths in addition to /usr/lib
# #   if I wanted to include libraries not in /usr/lib I'd specify
# #   their path using -Lpath, something like:
# LFLAGS = $(MEMORY_STYLE)/libremyprotos.a

# # define any libraries to link into executable:
# #   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
# #   option, something like (this will link in libmylib.so and libm.so:
# LIBS = -ljemalloc -lm -pthread -lprotobuf -lpthread -ljemalloc -ludt

# # define the C source files
# SRCS = random.cc memory.cc memoryrange.cc rat.cc whisker.cc whiskertree.cc socket.cc generator.cc remycc.cc tcp.cc

# # define the C object files 
# #
# # This uses Suffix Replacement within a macro:
# #   $(name:string1=string2)
# #         For each word in 'name' replace 'string1' with 'string2'
# # Below we are replacing the suffix .c of all words in the macro SRCS
# # with the .o suffix
# #
# OBJS = $(SRCS:.cc=.o)

# # define the executable file 
# MAIN = sender receiver prober

# #
# # The following part of the makefile is generic; it can be used to 
# # build any executable just by changing the definitions above and by
# # deleting dependencies appended to the file from 'make depend'
# #

# .PHONY: depend clean

# all:    $(MAIN)
# 		@echo  Done.

# $(MAIN): $(OBJS) 
# 		$(CC) $(CFLAGS) $(INCLUDES) -o $< $(OBJS) $(LFLAGS) $(LIBS)

# # this is a suffix replacement rule for building .o's from .c's
# # it uses automatic variables $<: the name of the prerequisite of
# # the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# # (see the gnu make manual section about automatic variables)
# .c.o:
# 		$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

# clean:
# 		$(RM) *.o *~ $(MAIN)

# depend: $(SRCS)
# 		makedepend $(INCLUDES) $^

# # DO NOT DELETE THIS LINE -- make depend needs it