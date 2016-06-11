#include "events.hh"

#include "pickle-packet.hh"

#include <cassert>
#include <cstring>

std::map<Time, AlarmEvent*> Event::alarms;
// Will be set in PktRecvEvent's constructor.
PktRecvEvent* Event::recv_event = nullptr;
UDPSocket* Event::socket = nullptr;

Event::Event() {}

void AlarmEvent::DoCallback(const Time& now) {
	for (const auto& x : listeners)
		x(now);
}

void PktRecvEvent::DoCallback(const TcpPacket& pkt) {
	for (const auto& x : listeners)
		x(pkt);
}

AlarmEvent::AlarmEvent()
	: listeners(),
	  when(),
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

void AlarmEvent::AddListener(void (*x)(Time now)) {
	listeners.push_back(x);
}

PktRecvEvent::PktRecvEvent(UDPSocket* socket)
	: listeners()
{
	assert(recv_event == nullptr);
	Event::recv_event = this;
	Event::socket = socket;
}

void PktRecvEvent::AddListener(void (*x)(TcpPacket pkt)) {
	listeners.push_back(x);
}

void Event::EventLoop() {
	constexpr int max_pkt_size = 2000;
	char buffer[max_pkt_size];
	sockaddr_in other_addr;
	PicklePacket pickle;
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
				x.second->DoCallback(now);
			}
		}
		else {
			// Packet received
			recv_event->DoCallback(pickle.Unpickle(buffer, strlen(buffer)));
		}
	}
}
