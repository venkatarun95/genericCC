//#include <netdb.h>
//#include <sys/types.h>
#include <arpa/inet.h>
#include <cassert>
#include <errno.h>
#include <iostream>
#include <string.h>

#include "udp-socket.hh"

using namespace std;

int UDPSocket::bindsocket(string s_ipaddr, int s_port, string myaddr /* __attribute((unused)) */, int myport /* __attribute((unused)) */){
	ipaddr = s_ipaddr;
	port = s_port;
	sockaddr_in my_addr;
	//memset((char *) &dest_addr, 0, sizeof(dest_addr));
	
	memset((char*) &my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(myport);
    if (inet_aton(myaddr.c_str(), &my_addr.sin_addr) == 0) 
    {
        std::cerr<<"inet_aton failed while binding to port. Check the format of given ip address. Code: "<<errno<<endl;
        return -1;
    }
    myport = myport*2;
    //memset((char *)&my_addr, 0, sizeof(my_addr)); my_addr.sin_family = AF_INET; my_addr.sin_addr.s_addr = htonl(INADDR_ANY); my_addr.sin_port = htons(0);
    if (bind(udp_socket, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0 ){
    	std::cerr<<"Failed to bind to port. Code: "<<errno<<endl;
        return -2;
    }
    //cout<<my_addr.sin_addr.s_addr<<" "<<my_addr.sin_port<<endl;

	bound = true;
	return 0;
}

int UDPSocket::bindsocket(int s_port)
{
	ipaddr = "";
	port = s_port;
	/*struct sockaddr_in ip4addr;
	int s;

	ip4addr.sin_family = AF_INET;
	ip4addr.sin_port = htons(port);

	// return values
	// 1  - success
	// 0  - wrong format of ipaddr
	// -1 - Internal error (ie. AF_INEt is wrong)
	if(inet_pton(AF_INET, ipaddr.c_str(), &ip4addr.sin_addr) == -1){
		std::cerr<<"Error while binding address. Code: "<<errno<<endl;
		//assert (false);
		return -1;
	}
 
 	int res = bind(s, (struct sockaddr*)&ip4addr, sizeof ip4addr);
 	// do something with res*/
 	sockaddr_in addr_struct;
	memset((char *) &addr_struct, 0, sizeof(addr_struct));
     
    addr_struct.sin_family = AF_INET;
    addr_struct.sin_port = htons(port);
    addr_struct.sin_addr.s_addr = htonl(INADDR_ANY);
 	if (bind(udp_socket , (struct sockaddr*)&addr_struct, sizeof(addr_struct) ) != 0){
 		std::cerr<<"Error while binding socket. Code: "<<errno<<endl;
 		return -1;
 	}
 	bound = true;
 	return 0;
}

ssize_t UDPSocket::senddata(char* data, ssize_t size, sockaddr_in *s_dest_addr){
	sockaddr_in dest_addr;
	memset((char *) &dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	if(s_dest_addr == NULL){
		assert(bound); // Socket not bound to an address. Please use 'bindsocket'

	    dest_addr.sin_port = htons(port);

	    if (inet_aton(ipaddr.c_str(), &dest_addr.sin_addr) == 0) 
	    {
	        std::cerr<<"inet_aton failed while sending data. Code: "<<errno<<endl;
	    }
	}
	else{
	    dest_addr.sin_port = ((struct sockaddr_in *)s_dest_addr)->sin_port;
	    dest_addr.sin_addr = ((struct sockaddr_in *)s_dest_addr)->sin_addr;
	}

	int res = sendto(udp_socket, data, size, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
	
	if ( res == -1 ){
		std::cerr<<"Error while sending datagram. Code: "<<errno<<std::endl;
		assert(false);
	}
	
	return res; // no of bytes sent
}

ssize_t UDPSocket::senddata(char* data, ssize_t size, string dest_ip, int dest_port){
	sockaddr_in dest_addr;
	memset((char *) &dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(dest_port);
    if (inet_aton(dest_ip.c_str(), &dest_addr.sin_addr) == 0) 
    {
        std::cerr<<"inet_aton failed while sending data. Code: "<<errno<<endl;
    }

    return senddata(data, size, &dest_addr);
}

// Modifies buffer to contain a null terminated string of the received data and returns the received buffer size (or -1 or 0, see below)
//
// Takes timeout in milliseconds. If timeout, returns NULL without changing the buffer. If data arrives before timeout, modifies buffer with the received data and returns the sender's address
// If timeout is negative, infinite timeout will be used. If it is 0, function will return immediately. Timeout will be rounded up to kernel time granularity,
// and kernel scheduling delays may cause actual timeout to exceed what is specified
//
// Functionality can be easily added to modify an argument to store the sender's address/ But since I am not using it yet, it is not implemented.
int UDPSocket::receivedata(char* buffer, int bufsize, int timeout, sockaddr_in &other_addr){
	assert(bound); // Socket not bound to an address. Please either use 'bind' or 'sendto'

    //sockaddr_in other_addr;
	unsigned int other_len;

	struct pollfd pfds[1];
	pfds[0].fd = udp_socket;
	pfds[0].events = POLLIN;

	int poll_val = poll(pfds, 1, timeout);
	if( poll_val == 1){
		if(pfds[0].revents & POLLIN){
			other_len = sizeof(other_addr);
			int res = recvfrom( udp_socket, buffer, bufsize, 0, (struct sockaddr*) &other_addr, &other_len );
			if ( res == -1 ){
				std::cerr<<"Error while receiving datagram. Code: "<<errno<<std::endl;
			}
			buffer[res] = '\0'; //terminating null character is not added by default

			//cout<<inet_ntoa(other_addr.sin_addr)<<" $@#^$ "<<ntohs(other_addr.sin_port)<<" "<<other_len<<endl;
			//return other_addr;
			return res;
		}
		else{
			std::cerr<<"There was an error while polling. Value of event field: "<<pfds[0].revents<<endl;
			return -1;
		}
	}
	else if ( poll_val == 0){
		return 0; //there was a timeout
	}
	else if ( poll_val == -1 ){
		if ( errno == 4 )
			return receivedata(buffer, bufsize, timeout, other_addr); //to make gprof work
		std::cerr<<"There was an error while polling. Code: "<<errno<<endl;
		return -1;
	}
	else{
		assert( false ); //should never come here
	}
}