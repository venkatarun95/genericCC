#ifndef HEADER_ACCESSOR_HH
#define HEADER_ACCESSOR_HH

#include "configs.hh"
#include "tcp-header.hh"

#include <vector>
#include <tuple>

// Abstract base class for classes that access the TCP
// headers. Provides interface to specify read/write permissions for
// various fields in the header.
class HeaderAccessor{
 public:
	HeaderAccessor() {}
  // Return list of (header_type, permissions) fields that will be
  // used for error checking and to determine in which order accessors should be called
  virtual std::vector< std::pair<HeaderMagicNum, AccessControlBits> >
  get_read_permissions() const = 0;
  virtual std::vector< std::pair<HeaderMagicNum, AccessControlBits> >
  get_write_permissions() const = 0;
  
	virtual void AccessHeader(TcpPacket& pkt) = 0;
};

#endif // HEADER_ACCESSOR_HH
