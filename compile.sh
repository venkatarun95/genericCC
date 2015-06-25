clear
clear
echo Compiling...

OPTIONS="-I./protobufs-loss-signal-remy -I./udt"

#g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT random.o -MD -MP -c -o random.o random.cc

g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT memory.o -MD -MP -c -o memory.o memory.cc

#g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT memoryrange.o -MD -MP -c -o memoryrange.o memoryrange.cc

#g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT rat.o -MD -MP -c -o rat.o rat.cc

#g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT whisker.o -MD -MP -c -o whisker.o whisker.cc

#g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT whiskertree.o -MD -MP -c -o whiskertree.o whiskertree.cc

#g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT udp-socket.o -MD -MP -c -o udp-socket.o udp-socket.cc

#g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT traffic-generator.o -MD -MP -c -o traffic-generator.o traffic-generator.cc

g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT remycc.o -MD -MP -c -o remycc.o remycc.cc

g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread                                  -Werror -fno-default-inline -pg -O2 -MT pcc-tcp.o -MD -MP -c -o pcc-tcp.o pcc-tcp.cc

g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT sender.o -MD -MP -c -o sender.o sender.cc

g++ -DHAVE_CONFIG_H -I. $OPTIONS -std=c++11 -pthread -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2 -MT receiver.o -MD -MP -c -o receiver.o receiver.cc



echo Linking sender...
g++ -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2   -o sender udp-socket.o pcc-tcp.o traffic-generator.o memoryrange.o memory.o rat.o whisker.o whiskertree.o random.o remycc.o sender.o ./protobufs-default/libremyprotos.a -ljemalloc -lm -pthread -lprotobuf -lpthread -ljemalloc -ludt

echo Linking receiver...
g++ -pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline -pg -O2   -o receiver random.o udp-socket.o receiver.o -ljemalloc -lm -pthread -lprotobuf -lpthread   -ljemalloc