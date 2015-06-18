#ifndef UDP_SOCKET_HH
#define UDP_SOCKET_HH

#include <string>

#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>

class UDPSocket{
private:
	int udp_socket;

	std::string ipaddr;
	int port;

	bool bound;
public:
	UDPSocket() : udp_socket(-1), ipaddr(), port(), bound(false) {
		udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	}

	int bindsocket(std::string ipaddr, int port, std::string myaddr, int myport);
	int bindsocket(int port);
	ssize_t senddata(char* data, ssize_t size, sockaddr_in *s_dest_addr);
	ssize_t senddata(char* data, ssize_t size, std::string dest_ip, int dest_port);
	int receivedata(char* buffer, int bufsize, int timeout, sockaddr_in &other_addr);
};

#endif