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

int main(){
	UDPSocket socket;
	socket.bindsocket(8888);
	char buff[BUFFSIZE];
	sockaddr_in sender_addr;

	chrono::high_resolution_clock::time_point start_time_point = chrono::high_resolution_clock::now();

	while(1){
		socket.receivedata(buff, BUFFSIZE, -1, sender_addr);
		////std::cout<<"Received: "<<buff<<std::endl;
		//std::cout<<"From: "<<inet_ntoa(sender_addr.sin_addr)<<":"<<ntohs(sender_addr.sin_port)<<std::endl;
		
		////std::chrono::high_resolution_clock::duration tp = std::chrono::high_resolution_clock::now().time_since_epoch();
		////std::cout<<"At: "<<(tp.count()) * std::chrono::high_resolution_clock::period::num / std::chrono::high_resolution_clock::period::den << std::endl<<std::endl;
		
		TCPHeader *header = (TCPHeader*)buff;
		header->receiver_timestamp = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start_time_point).count()*1000; //convert to milliseconds, the scale used everywhere

		socket.senddata(buff, sizeof(TCPHeader), &sender_addr);
	}
	return 0;
}