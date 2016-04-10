#ifndef PROCESS_HEADER_HH
#define PROCESS_HEADER_HH

#include "header-accessor.hh"
#include "tcp-header.hh"

#include <vector>

class ProcessHeader {
 public:
	// Note: construction could be computationally expensive if there
	// are many accessors. This does topological sort in runtime. Reuse
	// these objects.
	// TODO(venkat): add compile-time tologoical sort option?
	ProcessHeader(const std::vector<HeaderAccessor*>&);
	// Completes one processing cycle and returns the result.
	void Process(TcpPacket&);

 private:
	// True if b depends on result from a. Used in topological
	// sorting. Reports error if there is a conflict (ie. both write to
	// the same file)
	static bool HasEdge(const HeaderAccessor* a, const HeaderAccessor* b);

	// List of actions to be performed in topologically sorted order.
	std::vector<HeaderAccessor*> action_list;
};

#endif // PROCESS_HEADER_HH
