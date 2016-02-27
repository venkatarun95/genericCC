#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string.h>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "tcp.hh"
#include "udp-socket.hh"

#define BUFFSIZE 15000

using namespace std;

// used to lock socket used to listen for packets from the sender
mutex socket_lock;

// DEPRECATED
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
	assert(false); // DEPRECATED
	const int buffsize = 2048;
	char buff[buffsize];
	UDPSocket::SockAddress addr_holder;
	UDPSocket server_socket;

	server_socket.bindsocket(serverip, 4839, "0.0.0.0", 0);
	while (1) {
		this_thread::sleep_for( chrono::seconds(5) );

		server_socket.senddata(string("Listen").c_str(), 6, serverip, 4839);
		server_socket.receivedata(buff, buffsize, -1, addr_holder);

		char ip_addr[32];
		int port;
		sscanf(buff, "%s:%d", ip_addr, &port);

		int attempt_count = 0;
		const int num_attempts = 500;
		while(attempt_count < num_attempts) {
			socket_lock.lock();
				sender_socket.senddata("NATPunch", 8, string(ip_addr), port);
				// keep the timeout short so that acks are sent in time
				int received = sender_socket.receivedata(buff, buffsize, 5,
															addr_holder);
			socket_lock.unlock();
			if (received == 0) break;
			++ attempt_count;
		}
		cout << "Could not connect to sender at " << UDPSocket::decipher_socket_addr(addr_holder) << endl;
	}
}


double current_timestamp() {
	static chrono::high_resolution_clock::time_point start_time_point = \
		chrono::high_resolution_clock::now();
	return chrono::duration_cast<chrono::duration<double>>(
		chrono::high_resolution_clock::now() - start_time_point
	).count()*1000; //in milliseconds
}

// Set of TCP connections (via TcpConnection objects) on the receiver's
// side. Each connection is identified by ConnId type, a pair of src_id
// and flow_id.
class ConnectionSet {
 private:
	// (src_id, flow_id)
	typedef pair<unsigned, unsigned> ConnId;
	map<ConnId, TcpConnection> connections;

 public:
	ConnectionSet()
	:	connections()
	{}

	// Given a packet, returns the header to return, always assume the
	// packet to be of size 0. This is the only class interface.
	TcpHeader process_pkt(const TcpHeader& header);
};

TcpHeader ConnectionSet::process_pkt(const TcpHeader& header) {
	ConnId conn_id = make_pair(header.src_id, header.flow_id);
	if (connections.count(conn_id) == 0) { // new TCP connection
		connections.emplace(conn_id, TcpConnection(0, 0));
	}

	double cur_time = current_timestamp();
	connections[conn_id].register_received_packet(header, cur_time);
	// Transmit immediately, always
	TcpHeader response = connections[conn_id].get_next_pkt(cur_time, 0);
	assert(response.valid); // Receiver always acks
	if (connections[conn_id].get_state() == TcpConnection::ConnState::CLOSED) {
		connections.erase(conn_id);
	}
	return response;
}

// For each packet received, acks back the pseudo TCP header with the
// current  timestamp
void echo_packets(UDPSocket &sender_socket) {
	char buff[BUFFSIZE];
	sockaddr_in sender_addr;
	ConnectionSet connections;

	while (1) {
		int received __attribute((unused)) = -1;
		while (received == -1) {
			//socket_lock.lock();
				received = sender_socket.receivedata(buff, BUFFSIZE, -1, \
					sender_addr);
			//socket_lock.unlock();
			assert( received != -1 );
		}

		TcpHeader *header = (TcpHeader*)buff;
		*header = connections.process_pkt(*header);

		//socket_lock.lock();
			sender_socket.senddata(buff, sizeof(TcpHeader), &sender_addr);
		//socket_lock.unlock();
	}
}

int main(int argc __attribute((unused)), char* argv[] __attribute((unused))) {
	// if (argc != 2) {
	// 	cout << "Please specify the IP address of the NAT server" << endl;
	// 	return 0;
	// }
	// string nat_ip_addr = argv[1];

	UDPSocket sender_socket;
	sender_socket.bindsocket(8888);

	//thread nat_thread(punch_NAT, nat_ip_addr, ref(sender_socket));
	echo_packets(sender_socket);

	return 0;
}
