#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

void NetworkInterface::send_arp(uint16_t opcode, uint32_t sender_ip_address, uint32_t target_ip_address, EthernetAddress dst) {
  EthernetFrame frame;
  std::string buffer{};
  Serializer serializer{std::move(buffer)};
  frame.header.src = ethernet_address_;
  ARPMessage arpMessage;
  arpMessage.sender_ethernet_address = ethernet_address_;
  arpMessage.sender_ip_address = sender_ip_address;
  if (opcode == ARPMessage::OPCODE_REPLY) {
    arpMessage.target_ethernet_address = dst;
  } else {
    arpMessage.target_ethernet_address = {0, 0, 0, 0, 0, 0};
  }
  arpMessage.target_ip_address = target_ip_address;
  arpMessage.opcode = opcode;
  frame.header.type = EthernetHeader::TYPE_ARP;
  arpMessage.serialize(serializer);
  frame.payload = serializer.output();
  frame.header.dst = dst;
  transmit(frame);
}
//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  if (mapping_.count(next_hop.ipv4_numeric()) > 0 && dgram.header.ttl > 0) {
    // Create and send the frame right away
    EthernetFrame frame;
    std::string buffer{};
    Serializer serializer{std::move(buffer)};
    frame.header.src = ethernet_address_;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    dgram.serialize(serializer);
    frame.payload = serializer.output();
    frame.header.dst = mapping_[next_hop.ipv4_numeric()];
    transmit(frame);
  } else {
    // Broadcast ARP request, queue the datagram and wait for the reply of ARP
    std::pair<uint32_t, InternetDatagram> pair;
    pair.first = next_hop.ipv4_numeric();
    pair.second = dgram;
    pending_datagrams_.push(pair);
    if (pending_ip_.count(ip_address_.ipv4_numeric()) > 0) {
      if (timer_ - pending_ip_[ip_address_.ipv4_numeric()] < 5000) {
        return;
      } else {
        pending_ip_[ip_address_.ipv4_numeric()] = timer_;
        send_arp(ARPMessage::OPCODE_REQUEST, ip_address_.ipv4_numeric(), next_hop.ipv4_numeric(), ETHERNET_BROADCAST);
      }
    } else {
      pending_ip_[ip_address_.ipv4_numeric()] = timer_;
      send_arp(ARPMessage::OPCODE_REQUEST, ip_address_.ipv4_numeric(), next_hop.ipv4_numeric(), ETHERNET_BROADCAST);
    }
  }

}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if (frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_) {
    // Drop frame not destined of this interface
    return;
  }
  Parser parser{frame.payload};
  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    IPv4Datagram dgram{};
    dgram.parse(parser);
    datagrams_received_.push(dgram);
    return;
  } else if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arpMessage{};
    arpMessage.parse(parser);
    mapping_[arpMessage.sender_ip_address] = arpMessage.sender_ethernet_address;
    pending_timer_.emplace(timer_, arpMessage.sender_ip_address);
    while (!pending_datagrams_.empty()) {
      std::pair<uint32_t, InternetDatagram> qEle = pending_datagrams_.front();
      if (qEle.first == arpMessage.sender_ip_address) {
        InternetDatagram dgram = qEle.second;
        EthernetFrame dgramFrame;
        std::string buffer{};
        Serializer serializer{std::move(buffer)};
        dgramFrame.header.src = ethernet_address_;
        dgramFrame.header.type = EthernetHeader::TYPE_IPv4;
        dgram.serialize(serializer);
        dgramFrame.payload = serializer.output();
        dgramFrame.header.dst = arpMessage.sender_ethernet_address;
        transmit(dgramFrame);
        pending_datagrams_.pop();
      } else {
        break;
      }
    }
    for (auto it = pending_ip_.begin(); it != pending_ip_.end();) {
      if (it->first == arpMessage.target_ip_address) {
        pending_ip_.erase(it);
        break;
      } else {
        it++;
      }
    }
    if (arpMessage.opcode == ARPMessage::OPCODE_REQUEST && arpMessage.target_ip_address == ip_address_.ipv4_numeric()) {
      send_arp(ARPMessage::OPCODE_REPLY, ip_address_.ipv4_numeric(), arpMessage.sender_ip_address, arpMessage.sender_ethernet_address);
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  timer_ += ms_since_last_tick;
  while (!pending_timer_.empty() && !mapping_.empty() && timer_ - pending_timer_.front().first >= 30000) {
    mapping_.erase(pending_timer_.front().second);
    pending_timer_.pop();
  }
}
