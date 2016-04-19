#include "events.hh"

#include <cassert>

std::map<Time, Event*> Event::alarms;
Event* Event::recv_event = nullptr;
UDPSocket* Event::socket = nullptr;

Event::Event()
	: listeners()
{}

void Event::DoCallback() {
	for (const auto& x : listeners)
		x();
}

AlarmEvent::AlarmEvent()
	: when(),
		set(false)
{}

void AlarmEvent::SetAlarm(Time x) {
	UnsetAlarm();
	when = x;
	Event::alarms[when] = this;
	set = true;
}

void AlarmEvent::UnsetAlarm() {
	Event::alarms.erase(when);
	when = 0;
	set = false;
}

PktRecvEvent::PktRecvEvent(UDPSocket* socket) {
	assert(recv_event == nullptr);
	Event::recv_event = this;
	Event::socket = socket;
}

void Event::EventLoop() {
	constexpr int max_pkt_size = 2000;
	char buffer[max_pkt_size];
	sockaddr_in other_addr;
	while (true) {
		int timeout = int((double)alarms.begin()->first * 1000.0 - Time::Now());
		if (socket->receivedata(buffer, 
													 max_pkt_size,
													 timeout,
													 other_addr)) {
			// Timer expired
			Time now = Time::Now();
			for (auto& x : alarms) {
				if (x.first > now)
					break;
				x.second->DoCallback();
			}
		}
		else {
			// Packet received
			recv_event->DoCallback();
		}
	}
}
