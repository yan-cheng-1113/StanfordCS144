#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return seqno_ - cur_ackno_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return rtrns_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  (void)transmit;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage empty_message {zero_ + seqno_, false, "", false, input_.has_error()};
  return empty_message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if (msg.RST) {
    input_.set_error();
  }
  if (msg.window_size > 0) {
    wnd_size_ = msg.window_size;
  } else {
    wnd_size_ = 1;
    zero_wnd_ = true;
  }
  Wrap32 new_ackno = msg.ackno.value();
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  rtrns_timer_ += ms_since_last_tick;
  if(rtrns_timer_ >= RTO_ms_) {
     if (wnd_size_ != 1 && !zero_wnd_) {
      RTO_ms_ *= 2;
    }
    else {
      RTO_ms_ = initial_RTO_ms_;
    }
    rtrns_timer_ = 0; 
  }
 

}
