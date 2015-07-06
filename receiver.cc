#include <iostream>
#include <string.h>
#include <chrono>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "tcp-header.hh"
#include "udp-socket.hh"

#define BUFFSIZE 1500

using namespace std;

// used to lock socket used to listen for packets from the sender
mutex socket_lock; 

// Periodically probes a server with a well known IP address and looks for
// senders that want to connect with this application. In case such a 
// sender exists, tries to punch holes in the NAT (assuming that this 
// process is a NAT).
//
// To be run as a separate thread, so the rest of the code can assume that 
// packets arrive as if no NAT existed.
//
// Currently is only robust if the sender is not behind a NAT, but only the 
// sender needs to be changed to fix this. 
void punch_NAT(string serverip, UDPSocket &sender_socket) {
	const int bufsize;
	char buff[bufsize];
	sockaddr_in addr_holder;
	UDPSocket server_socket;

	server_socket.bindsocket(serverip, 4839, "0.0.0.0", 0);
	while (1) {
		this_thread::sleep_for( chrono::seconds(5) );

		server_socket.senddata("Listen", 6, serverip, 4839);
		server_socket.receivedata(buff, bufsize, -1, addr_holder);

		char ip_addr[32];
		int port;
		sscanf(buff, "%s %d", ip_addr, &port);

		int attempt_count = 0;
		while(attempt_count < 500) {
			socket_lock.lock();
				sender_socket.senddata("NATPunch", 8, string(ip_addr), port);
				// keep the timeout short so that acks are sent in time
				int received = sender_socket.receivedata(buff, buffsize, 5, 
															addr_holder);
			socket.unlock()
			if (received == 0) break;
			++ attempt_count;
		}
	}
}

// For each packet received, acks back the pseudo TCP header with the 
// current  timestamp
void echo_packets(UDPSocket &sender_socket) {
	char buff[BUFFSIZE];
	sockaddr_in sender_addr;

	chrono::high_resolution_clock::time_point start_time_point = \
		chrono::high_resolution_clock::now();

	while (1) {
		int received = -1;
		while (received != 0) {
			socket_lock.lock();
				received = sender_socket.receivedata(buff, BUFFSIZE, 5000, sender_addr);
			socket_lock.unlock();
		}

		TCPHeader *header = (TCPHeader*)buff;
		header->receiver_timestamp = \
			chrono::duration_cast<chrono::duration<double>>(
				chrono::high_resolution_clock::now() - start_time_point
			).count()*1000; //in milliseconds

		socket_lock.lock();
			sender_socket.senddata(buff, sizeof(TCPHeader), &sender_addr);
		socket_lock.unlock();
	}
}

int main() {
	UDPSocket sender_socket;
	socket.bindsocket(8888);
	
	thread nat_thread(punch_NAT, socket);
	echo_packets(socket);

	return 0;
}