#include "process-header.hh"

#include <cassert>
#include <cstdlib>
#include <set>
#include <vector>

using namespace std;

bool ProcessHeader::HasEdge(const HeaderAccessor* a, const HeaderAccessor* b) {
	// For small number of headers, n^2 sort is fastest.
	bool depends = false;
	for (const auto& x : a->get_write_permissions()) {
		for (const auto& y : b->get_read_permissions()) {
			if (x.first == y.first)
				depends = true;
		}
	}
	
	// Check for WaW conflicts
	for (const auto& x : a->get_write_permissions()) {
		for (const auto& y : b->get_write_permissions()) {
			if (x.first == y.first) {
				assert (false); // Write after Write conflict
				exit(1);
			}	
		}
	}

	// TODO(venkat): make sure no accessor wants to read and write to
	// the same field.

	return depends;
}

ProcessHeader::ProcessHeader(const std::vector<HeaderAccessor*>& given) 
	: action_list()
{
	// Find all edges.
	// The i^th set contains vertex ids for the i^th vertex.
	vector< set<int> > incoming_edges, outgoing_edges;
	for (unsigned i = 0; i < given.size(); ++ i) {
		for (unsigned j = 0; j < given.size(); ++ j) {
			if (i != j && ProcessHeader::HasEdge(given[i], given[j])) {
				incoming_edges[j].insert(i);
				outgoing_edges[i].insert(j);
			}
		}
	}
	
	// Vertices with no incoming edges
	set<int> next_candidates;
	for (unsigned i = 0; i < given.size(); ++ i) {
		if (incoming_edges[i].size() == 0)
			next_candidates.insert(i);
	}

	assert(!next_candidates.empty());

	// Do topological sort
	while (!next_candidates.empty()) {
		int i = *next_candidates.begin();
		action_list.push_back(given[i]);
		for (const int j : outgoing_edges[i]) {
			incoming_edges[j].erase(i);
			if (incoming_edges[j].empty())
				next_candidates.insert(j);
		}
	}

	assert(action_list.size() == given.size()); // Check for loops in
																							// dependancy graph
}

void ProcessHeader::Process(TcpPacket& pkt) {
	for (auto& x : action_list) {
		x->AccessHeader(pkt);
	}
}
