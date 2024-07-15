#pragma once

#include <memory>
#include <optional>

#include "exception.hh"
#include "network_interface.hh"

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // Compare the prefixes of two ip addresses
  bool prefix_equal(uint32_t ip1, uint32_t ip2, uint8_t prefix_len) 
  {
    uint32_t offset = (prefix_len == 0) ? 0 : 0xffffffff << (32 - prefix_len);
    return (ip1 & offset) == (ip2 & offset);
  }  
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};
  struct RoutingItem {
    uint32_t route_prefix_ {0};
    uint8_t prefix_length_ {0};
    optional<Address> next_hop_ {nullopt};
    size_t interface_num_ {0}; 
  };
  std::vector<RoutingItem> routing_table_ {};
};
