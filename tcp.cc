#include "tcp.h"

using namespace std;

int TcpConnection::register_received_packet(TCPHeader pkt) {
	assert(window[window_pos] == false);
	assert(seq_num - expected_seq_no < window.size());
	if (seq_num == expected_seq_no) {
		window_pos = (window_pos + 1) % window.size();
		expected_seq_no ++;
		while (window[window_pos] == true) {
			window[window_pos] = false;
			window_pos = (window_pos + 1) % window.size();
			expected_seq_no ++;
		}
	}
	else {
		window[(window_pos + seq_num - expected_seq_no)%window.size()] = true;
	}
	return expected_seq_no;
}

void TcpConnection::interactive_test() {
	int window;
	cout << "Enter window size: ";
	cin >> window;
	TcpConnection connection(window);
	while (true) {
		unsigned seq_num;
		cin >> seq_num;
		cout << connection.register_ack(seq_num);
	}
}
