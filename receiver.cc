#include <iostream>
#include <string.h>
#include <chrono>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "udp-socket.hh"

#define BUFFSIZE 1500

struct TCPHeader{
	int seq_num;
	int flow_id;
	int src_id;
	double sender_timestamp;
}header, ack_header;

int main(){
	UDPSocket socket;
	socket.bindsocket(8888);
	char buff[BUFFSIZE];
	sockaddr_in sender_addr;
	while(1){
		//memset(buff,'\0', BUFFSIZE);
		//sockaddr_in sender_addr = socket.receivedata(buff, BUFFSIZE);
		socket.receivedata(buff, BUFFSIZE, -1, sender_addr);
		////std::cout<<"Received: "<<buff<<std::endl;
		//std::cout<<"From: "<<inet_ntoa(sender_addr.sin_addr)<<":"<<ntohs(sender_addr.sin_port)<<std::endl;
		
		////std::chrono::high_resolution_clock::duration tp = std::chrono::high_resolution_clock::now().time_since_epoch();
		////std::cout<<"At: "<<(tp.count()) * std::chrono::high_resolution_clock::period::num / std::chrono::high_resolution_clock::period::den << std::endl<<std::endl;
		
		//Prepare data to be sent
		//char ip_to_send[32];
		//int seq_num, flow_id, src_id;//, port_to_send;
		//double cur_time;
		//char packet_data[BUFFSIZE];
		
		//sscanf( buff, "%d %d %d %lf %s", &seq_num, &flow_id, &src_id, &cur_time, /*ip_to_send, &port_to_send,*/ packet_data );
		//sprintf( buff, "%d %d %d %*lf", seq_num, flow_id, src_id, 20, cur_time );

		//buff[ sizeof(TCPHeader) / sizeof(char) ] = '\0';
		//TCPHeader header;
		//memcpy(&header, buff, sizeof(TCPHeader));
		
		//printf( "Sending: %s\nTo %s:%d\n", buff, ip_to_send, port_to_send );
		socket.senddata(buff, sizeof(TCPHeader), &sender_addr);
		//socket.senddata(buff, ip_to_send, port_to_send);
	}
	return 0;
}