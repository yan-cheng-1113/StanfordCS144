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
    zero_wnd_ = false;
  } else {
    wnd_size_ = 1;
    zero_wnd_ = true;
  }
  if(msg.ackno.has_value()) {
    Wrap32 new_ackno = msg.ackno.value();
    if (unwrap_num(new_ackno) > cur_ackno_) {
      cur_ackno_ = unwrap_num(new_ackno);
    }else {
      return;
    }
    // Remove acked segments
    while(!outstanding_segs_.empty() 
          && cur_ackno_ >= unwrap_num(outstanding_segs_.front().seqno)) {
            outstanding_segs_.pop();
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  if (outstanding_segs_.empty()) {
    rtrns_cnt_ = 0;
    RTO_ms_ = initial_RTO_ms_;
    return;
  }
  rtrns_timer_ += ms_since_last_tick;
  if(rtrns_timer_ >= RTO_ms_) {
    if (wnd_size_ != 1 && !zero_wnd_) {
      RTO_ms_ *= 2; // Double the retransmission time
    }
    else if (zero_wnd_){
      RTO_ms_ = initial_RTO_ms_; 
    }
    transmit(outstanding_segs_.front()); // Retransmit the oldest unACKed segment
    rtrns_cnt_ ++;
    rtrns_timer_ = 0; 
  }
}