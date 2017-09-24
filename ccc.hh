#ifndef CCC_HH
#define CCC_HH

#include<iostream>

class CCC
{
public:
  CCC() : 
  _intersend_time( 0 ),
  _the_window( 2 ),
  _timeout( 1000 ) {}

  virtual ~CCC() {}
public:
  virtual void init() {}
  virtual void close() {}
  
  // Note: for onAck and for onPktSent, each group of (see configs.hh)
  // NUM_PACKETS_PER_LINK_RATE_MEASUREMENT group of packets is treated as one 
  // packet. Therefore some congestion control protocols (such as RemyCC) must
  // compensate for this. This decision was made so that the 'send_ewma' and 
  // 'recv_ewma' are not messed with because of the deliberate packet bunching
  virtual void onACK( int ack __attribute((unused)), 
    double receiver_timestamp __attribute((unused)), double sent_time __attribute((unused)) ) {std::cout<<"Hello!";}
  virtual void onPktSent( int seq_num __attribute((unused)) ) { }
  virtual void onDupACK() {}
  virtual void onTimeout() {}
  
  virtual void onLinkRateMeasurement( double measured_link_rate __attribute((unused)) ) {}
  //virtual void onPktReceived(const CPacket* pkt) {}
  //virtual void processCustomMsg(const CPacket& pkt) {}

  double get_the_window(){ return _the_window; }
  double get_intersend_time(){ return _intersend_time; }
  double get_timeout(){ return _timeout; }
  
  void set_timestamp(double) {}
  void set_min_rtt(double) {}

protected:
  void setRTO(const int& usRTO){ _timeout = usRTO/1000.0;}

  double _intersend_time; // in ms
  double _the_window;
  double _timeout; // in ms
}; 

#endif
