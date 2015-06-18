#ifndef REMYCC_HH
#define REMYCC_HH

#include <chrono>
#include <unordered_map>
#include <vector>

#include "ccc.hh"

#include "rat.hh"
#include "whiskertree.hh"
#include "packet.hh"

class RemyCC: public CCC
{
private:
	WhiskerTree tree; // WARNING: it is bad practice to use pointers
	Rat rat;

	std::chrono::high_resolution_clock::time_point start_time_point;
	std::unordered_map<int, double> unacknowledged_packets;

	int flow_id;

	double current_timestamp();

protected:

public:

   virtual void init();
   virtual void onACK(int ack) override ;//{ std::cout<<"Acked "<<ack<<" "; }
   //virtual void onPktReceived(const int& seq_num){ std::cout<<"Packet Received\n"; }
   //virtual void onLoss(const int* losslist, const int& size) { std::cout<<"Packet Lost\n"; }
   //virtual void onTimeout() { std::cout<<"Timeout!\n"; }
   virtual void onPktSent(int seq_num) override ;//{ std::cout<<"Packet Sent"; }

	/*RemyCC () : tree( nullptr ), rat( nullptr ), start_time_point(), unacknowledged_packets(), flow_id( 0 ) {
		_the_window = 2; 
      _intersend_time=1000000;
	}*/
	RemyCC( WhiskerTree & s_tree ) 
	: 	tree( s_tree ), rat( tree ), start_time_point(), unacknowledged_packets(), flow_id( 0 ) 
	{
		_the_window = 2; 
      _intersend_time=0;
	}
};

#endif