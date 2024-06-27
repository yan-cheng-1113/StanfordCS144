#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return unwrap_num(seqno_) - unwrap_num(cur_ackno_);
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return rtrns_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  TCPSenderMessage tcpSenderMessage {};
  string payload_ {};
  read(input_.reader(), wnd_size_, payload_);
  if (payload_.size() > wnd_size_) {
    payload_ = payload_.substr(0, wnd_size_);
  }
  if(!sync_) {
    sync_ = true;
    tcpSenderMessage.SYN = true;
  } else {
    tcpSenderMessage.SYN = false;
    if (payload_.empty()) {
      return;
    }
  }
  tcpSenderMessage.seqno = seqno_;
  tcpSenderMessage.payload = payload_;
  tcpSenderMessage.FIN = input_.reader().is_finished();
  tcpSenderMessage.RST = input_.has_error();
  transmit(tcpSenderMessage);
  outstanding_segs_.push(tcpSenderMessage);
  wnd_size_ -= payload_.size();
  seqno_ = seqno_ + tcpSenderMessage.sequence_length();
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage empty_message {seqno_, false, "", false, input_.has_error()};
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
    if (unwrap_num(new_ackno) <= unwrap_num(cur_ackno_)) {
      return;
    }
    if (unwrap_num(new_ackno) > unwrap_num(seqno_)) {
      return;
    }
    cur_ackno_ = new_ackno;
    RTO_ms_ = initial_RTO_ms_;
    // Remove acked segments
    while(!outstanding_segs_.empty() 
          && unwrap_num(cur_ackno_) > unwrap_num(outstanding_segs_.front().seqno)) {
            outstanding_segs_.pop();
    }
    if(fin) {
      fin_ack_ = true;
    }
    rtrns_cnt_ = 0;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  if (rtrns_cnt_ > TCPConfig::MAX_RETX_ATTEMPTS) {
    return;
  }
  if (outstanding_segs_.empty()) {
    rtrns_cnt_ = 0;
    RTO_ms_ = initial_RTO_ms_;
    return;
  }
  if (rtrns_timer_ != 0) {
    rtrns_timer_ += ms_since_last_tick;
  }else {
    rtrns_timer_ = ms_since_last_tick;
  }
  
  if(rtrns_timer_ >= RTO_ms_) {
    transmit(outstanding_segs_.front()); // Retransmit the oldest unACKed segment
    if (!zero_wnd_) {
      RTO_ms_ *= 2; // Double the retransmission time
    }
    // else if (zero_wnd_){
    //   RTO_ms_ = initial_RTO_ms_; 
    // }
    rtrns_cnt_ ++;
    rtrns_timer_ = 0; 
  }
}