#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

using namespace std;
class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

  // Convert a Wrap32 type into uint64_t type
  uint64_t unwrap_num ( const Wrap32& num ) const { return num.unwrap(isn_, this->input_.reader().bytes_popped()); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  Wrap32 zero_ {0}; // 0 in Wrap32
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms_ {initial_RTO_ms_};
  uint64_t rtrns_timer_ {0};
  queue<TCPSenderMessage> outstanding_segs_ {};
  uint64_t seqno_ {0};
  uint64_t cur_ackno_ {0};
  uint64_t rtrns_cnt_ {0};
  uint16_t wnd_size_ {1};
  bool zero_wnd_ {false};
};
