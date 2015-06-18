#ifndef CCC_HH
#define CCC_HH

#include<iostream>

class CCC
{
public:
  CCC() : 
  _intersend_time( 0 ),
  _the_window( 2 ),
  _timeout( 0.002 ) {}

  virtual ~CCC() {}
public:
  virtual void init() {}
  virtual void close() {}
  virtual void onACK( int ack __attribute((unused)) ) {std::cout<<"Hello!";}
  //virtual void onLoss(const int* losslist, const int& size) {}   virtual void onTimeout() {}
  virtual void onPktSent( int seq_num __attribute((unused)) ) { }
  virtual void onTimeout() {}
  //virtual void onPktReceived(const CPacket* pkt) {}
  //virtual void processCustomMsg(const CPacket& pkt) {}

  double get_the_window(){ return _the_window; }
  double get_intersend_time(){ return _intersend_time; }
  double get_timeout(){ return _timeout; }

protected:
  void setRTO(const int& usRTO){ _timeout = usRTO/1000.0;}

  double _intersend_time;
  double _the_window;

private:
  double _timeout;
}; 

#endif