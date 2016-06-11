#ifndef EVENTS_HH
#define EVENTS_HH

#include "configs.hh"
#include "tcp-header.hh"
#include "udp-socket.hh"

#include <chrono>
#include <map>
#include <vector>

class AlarmEvent;
class PktRecvEvent;

// Base class for events. When event is fired, all callback functions
// are called one by one in the order in which they were added. Keeps
// a global track of all objects of its subclasses.
class Event {
 public:
	Event();
	virtual ~Event() {;}

	// Must be run a a thread for the events system to function. This
	// should not be expected to return.
	static void EventLoop();

 protected:
	// Alarms need to be set a these times.
	static std::map<Time, AlarmEvent*> alarms;
	// Event that fires when a packet is received.
	static PktRecvEvent* recv_event;

	static	UDPSocket* socket;
};

// Fires callbacks at a set time. Handles atmost one alarm at a time,
// multiple objects can be used for multiple alarms.
//
// Note that in some platforms, due to OS scheduling errors, the alarm
// may fire later (but never before) the the set time.
class AlarmEvent : public Event {
 public:
	AlarmEvent();
	// Remove alarm if set.
	void UnsetAlarm();
	// Set a timer, overriding any previous setting.
	void SetAlarm(Time);
	// Call all callback functions.
	void DoCallback(const Time& now);
	// Add a callback function.
	void AddListener(void (*x)(Time now));

 private:
	// List of callback functions.
	std::vector<void (*)(Time now)> listeners;
	// Next alarm time.
	Time when;
	// Whether a timer has been set.
	bool set;
};

class PktRecvEvent : public Event {
 public:
	PktRecvEvent(UDPSocket*);
	// Call all callback functions.
  void DoCallback(const TcpPacket& pkt);
	// Add a callback function.
	void AddListener(void (*x)(TcpPacket pkt));

 private:
	// List of callback functions.
	std::vector<void (*)(TcpPacket pkt)> listeners;
};

#endif // EVENTS_HH
