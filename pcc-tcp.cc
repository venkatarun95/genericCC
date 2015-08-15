#ifndef WIN32
   #include <unistd.h>
   #include <cstdlib>
   #include <cstring>
   #include <netdb.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <wspiapi.h>
#endif
#include <iostream>
#include "udt/udt.h"
#include "pcc.h"

#include <cassert>
#include <chrono>
#include <mutex>

#include "pcc-tcp.hh"

using namespace std;

#ifndef WIN32
void* monitor(void*);
#else
DWORD WINAPI monitor(LPVOID);
#endif

double PCC_TCP::current_timestamp( chrono::high_resolution_clock::time_point &start_time_point ){
   using namespace chrono;
   high_resolution_clock::time_point cur_time_point = high_resolution_clock::now();
   return duration_cast<duration<double>>(cur_time_point - start_time_point).count()*1000; //convert to milliseconds, because that is the scale on which the rats have been trained
} 

void PCC_TCP::send_data( double flow_size, bool byte_switched, int flow_id, int src_id )
{
   assert(byte_switched);
  // use this function to initialize the UDT library
   UDT::startup();

   flow_id = flow_id; src_id = src_id;

   struct addrinfo hints, *local, *peer;

   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   //hints.ai_socktype = SOCK_DGRAM;

   if (0 != getaddrinfo(NULL, "9000", &hints, &local))
   {
      cout << "incorrect network address.\n" << endl;
      return;
   }

   UDTSOCKET client = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);

   // UDT Options
   UDT::setsockopt(client, 0, UDT_CC, new CCCFactory<BBCC>, sizeof(CCCFactory<BBCC>));
   //UDT::setsockopt(client, 0, UDT_MSS, new int(9000), sizeof(int));
   //UDT::setsockopt(client, 0, UDT_SNDBUF, new int(10000000), sizeof(int));
   //UDT::setsockopt(client, 0, UDP_SNDBUF, new int(10000000), sizeof(int));

   // Windows UDP issue
   // For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
   #ifdef WIN32
      UDT::setsockopt(client, 0, UDT_MSS, new int(1052), sizeof(int));
   #endif

   /*
   UDT::setsockopt(client, 0, UDT_RENDEZVOUS, new bool(true), sizeof(bool));
   if (UDT::ERROR == UDT::bind(client, local->ai_addr, local->ai_addrlen))
   {
      cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   */

   freeaddrinfo(local);

   if (0 != getaddrinfo(dstaddr.c_str(), to_string( dstport ).c_str(), &hints, &peer))
   {
      cout << "incorrect server/peer address. " << dstaddr << ":" << dstport << endl;
      return;
   }

   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(client, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
      return;
   }
   freeaddrinfo(peer);

   // using CC method
   BBCC* cchandle = NULL;
   int temp;
   UDT::getsockopt(client, 0, UDT_CC, &cchandle, &temp);
//   if (NULL != cchandle)
//      cchandle->setRate(1);

   int size = flow_size; //CHANGE - by venkat
   char* data = new char[size];

   // used to allow this function to return after the monitor has printed
   mutex run_monitor; 
   run_monitor.lock();
   struct {
      UDTSOCKET* client;
      mutex* run_monitor;
   } compound = {&client, &run_monitor};

   #ifndef WIN32
      pthread_create(new pthread_t, NULL, monitor, &compound);
   #else
      CreateThread(NULL, 0, monitor, &client, 0, NULL, &run_monitor);
   #endif

   chrono::high_resolution_clock::time_point start_time_point = chrono::high_resolution_clock::now();
   // while ( )
   // {
      int ssize = 0;
      int ss;
      while (ssize < size)
      {
         if (UDT::ERROR == (ss = UDT::send(client, data + ssize, size - ssize, 0)))
         {
            cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
            break;
         }
         ssize += ss;
      }
      bool tmp = false;
      UDT::getsockopt(client, 0, UDT_SNDSYN, &tmp, new int(0));
      // cout << "Sockopt: " << tmp << endl;
      // cout << current_timestamp( start_time_point ) << " " << duration << endl;

   //    if (ssize < size)
   //       break;
   // }

   UDT::close(client);

   run_monitor.try_lock(); // allow monitor to run once

   delete [] data;

   return;
}

#ifndef WIN32
void* monitor(void* s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
   struct TmpCompound{
      UDTSOCKET* client;
      mutex* run_monitor; // useless for now
   } compound = *(TmpCompound*)s;

   UDTSOCKET u = *(UDTSOCKET*)compound.client;

   UDT::TRACEINFO perf;

   // cout << "SendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tRecvNAK" << endl;
   while (true)//!((mutex*)compound.run_monitor)->try_lock())
   {
      #ifndef WIN32
         usleep(100000);
      #else
         Sleep(100);
      #endif
      if (UDT::ERROR == UDT::perfmon(u, &perf))
      {
         // cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
         break;
      }
      // cout << perf.mbpsSendRate << "\t\t"
      //      << perf.msRTT << "\t"
      //      <<  perf.pktSentTotal << "\t"
      //      << perf.pktSndLossTotal << "\t\t\t"
      //      << perf.pktRecvACKTotal << "\t"
      //      << perf.pktRecvNAKTotal << endl;
   };
   std::cout<<"\n\nData Successfully Transmitted\n\tThroughput: "<<\
      perf.mbpsRecvRate/8<<" bytes/sec\n\tAverage Delay: "<<perf.msRTT/100.0\
      <<" sec/packet\n";

   double avg_throughput = -1;
   double avg_delay = -1;
   std::cout<<"\n\tAvg. Throughput: "<<avg_throughput<<" bytes/sec\n\tAverage Delay: "<<avg_delay<<" sec/packet\n";

   ((mutex*)compound.run_monitor)->unlock();

   #ifndef WIN32
      return NULL;
   #else
      return 0;
   #endif
}

PCC_TCP::~PCC_TCP() {
   // use this function to release the UDT library
   UDT::cleanup();
}