#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  routing_table_.push_back(RoutingItem{route_prefix, prefix_length, next_hop, interface_num});
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.

  // Interfaces empty, return
  if (_interfaces.empty()) {
    return;
  }
  // Loop through all the interfaces
   for (auto it = _interfaces.begin(); it != _interfaces.end(); it++) {
    bool drop = true;
    // Get datagrams of the current interface
    std::queue<InternetDatagram> datagrams = it->get()->datagrams_received();
    while (!datagrams.empty()) {
      InternetDatagram datagram = datagrams.front();
      if (datagram.header.ttl <= 1) {
        datagrams.pop();
        break;
      }
      uint8_t longest_Prefix = 0; // longest matching prefix
      optional<Address> next_hop = nullopt;
      size_t interface_num = 0;
      for (auto routeItem = routing_table_.begin(); routeItem != routing_table_.end(); routeItem++) {
        if (prefix_equal(datagram.header.dst, routeItem->route_prefix_, routeItem->prefix_length_)) {
          if (routeItem->prefix_length_ > longest_Prefix) {
            drop = false;
            longest_Prefix = routeItem->prefix_length_;
            next_hop = routeItem->next_hop_;
            interface_num = routeItem->interface_num_;
          }
        }
      }
      if (drop) {
        datagrams.pop();
        break;
      }
      datagram.header.ttl -= 1;
      if (next_hop.has_value()) {
        _interfaces[interface_num]->send_datagram(datagram, next_hop.value());
      } else {
        _interfaces[interface_num]->send_datagram(datagram, Address::from_ipv4_numeric(datagram.header.dst));
      }
      break;
    }
  }

}
