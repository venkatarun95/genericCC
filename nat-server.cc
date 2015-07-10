#include <chrono>
#include <string>
#include <vector>

#include "udp-socket.hh"

using namespace std;
using namespace chrono;

#define BUFF_SIZE 2048

int main() {
  char buff[BUFF_SIZE];
  sockaddr_in sender_addr;
  UDPSocket::SockAddr socket;
  socket.bindsocket(8888);

  // IP addresses and ports of all clients' wanting to connect to 
  // listeners. If the clients also implement proper UDP punching, they
  // can be behind NATs too and not register as a listener.
  vector< string, int > connect_requests; 
  // all clients registered for listening. Clients behind a NAT will do this
  unordered_map< sockaddr_in, time_point< system_clock > listeners;

  while (true) {
    socket.receivedata(buff, BUFF_SIZE, -1, sender_addr);
    if (string(buff) == "Listen") {
      listeners.emplace(sender_addr, system_clock::now() );
    }
    else if (string(buff) == "Connect") {
      if (listeners.empty()) {
        socket.senddata("NoListeners", 11, sender_addr);
      }
      else {
        // Send a list of listeners to the client
        string res;
        for (const auto & x : listeners)
          res += UDPSocket::decipher_socket_addr(x.first) + " ";
        socket.senddata(res.c_str(), res.size(), sender_addr);

        // Notify the listeners so that they may punch the NAT on their end
        string client_addr = UDPSocket::decipher_socket_addr(sender_addr);
        for (const auto & x : listeners)
          sockaddr_in.senddata(client_addr.c_str(), client_addr.size(), sender_addr);
      }
    }
  }

  return 0;
}