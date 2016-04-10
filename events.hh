#ifndef EVENTS_HH
#define EVENTS_HH

#include "configs.hh"
#include "udp-socket.hh"

#include <chrono>
#include <map>
#include <vector>

// Base class for events. When event is fired, all callback functions
// are called one by one in the order in which they were added. Keeps
// a global track of all objects of its subclasses.
class Event {
 public:
	Event();
	void AddListener();
	virtual ~Event() {;}

	// Must be run a a thread for the events system to function. This
	// should not be expected to return.
	static void EventLoop();

 protected:
	// Alarms need to be set a these times.
	static std::map<Time, Event*> alarms;
	// Event that fires when a packet is received.
	static Event* recv_event;

	// Calls all the callback functions.
	void DoCallback();
	static	UDPSocket* socket;

 private:
	// List of callback functions.
	std::vector<void (*)()> listeners;
	// Current time in ms.
	static double current_timestamp();
	// When the event loop was started.
	static std::chrono::high_resolution_clock::time_point start_time_point;
};

// Fires callbacks at a set time. Handles atmost one alarm at a time,
// multiple objects can be used for multiple alarms.
class AlarmEvent : public Event {
 public:
	AlarmEvent();
	// Remove alarm if set.
	void UnsetAlarm();
	// Set a timer, overriding any previous setting.
	void SetAlarm(Time);

 private:
	Time when;
	// Whether a timer has been set.
	bool set;
};

class PktRecvEvent : public Event {
 public:
	PktRecvEvent(UDPSocket*);
};

#endif // EVENTS_HH
