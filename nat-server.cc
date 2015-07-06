#include <string>
#include <vector>

#include "udp-socket.hh"

#define BUFF_SIZE 2048

int main() {
	char buff[BUFF_SIZE];
	sockaddr_in sender_addr;
	UDPSocket socket;
	socket.bindsocket(8888);

	// all clients' ip address and port wanting to connect to listeners.
	vector< string, int > connect_requests; 
	// all clients registered for listening
	vector< sockaddr_in > listeners;

	while (true) {
		socket.receivedata(buff, BUFF_SIZE, -1, sender_addr);
		if (string(buff) == "Listen") {
			listeners.push_back(sender_addr);
		}
		else if (string(buff) == "Connect") {
			if (listeners.empty()) {
				socket.senddata("NoListeners", 11, sender_addr);
			}
			else {
				
			}
		}
	}

	return 0;
}