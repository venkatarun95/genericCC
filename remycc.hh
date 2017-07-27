#ifndef REMYCC_HH
#define REMYCC_HH

#include <chrono>
#include <unordered_map>
#include <vector>

#include "ccc.hh"

#include "configs.hh"
#include "rat.hh"
#include "whiskertree.hh"
#include "packet.hh"

class RemyCC: public CCC {
private:
	WhiskerTree tree;
	Rat rat;

	std::chrono::high_resolution_clock::time_point start_time_point;
	std::unordered_map<int, double> unacknowledged_packets;

	int flow_id;
	double cur_tick;
	double current_timestamp();

	double measured_link_rate;

protected:

public:

	virtual void init();
	virtual void onACK(int ack, double receiver_timestamp, double sender_timestamp __attribute((unused))) override ;
	virtual void onPktSent(int seq_num) override ;
	virtual void onTimeout() override { std::cerr << "Ack timed out!\n"; }
	virtual void onLinkRateMeasurement( double s_measured_link_rate ) override;
	void set_timestamp(double s_cur_tick) {cur_tick = s_cur_tick;}

	RemyCC( WhiskerTree & s_tree ) 
	  : 	tree( s_tree ), rat( tree ), start_time_point(), unacknowledged_packets(), flow_id( 0 ), cur_tick( 0 ), measured_link_rate( -1 ) 
	{
		_the_window = 2;
		_intersend_time = 0;
		_timeout = 1000;
	}
};

#endif
