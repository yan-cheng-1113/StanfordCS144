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

bool TCPSender:: FIN_sent (uint64_t pay_size)
{
  // If the writer has been closed, check if there is space for fin. 
    // To avoid mutilpe fins, check for max_payload_size
    if (input_.writer().is_closed() 
        && pay_size < wnd_size_ 
        && pay_size <= TCPConfig::MAX_PAYLOAD_SIZE) {
      // If fin has been sent, just return
      if (fin_sent_) {
        return true;
      }
      fin_ = true;
    }
    return false;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  uint64_t pay_size = reader().bytes_buffered();
  if (FIN_sent(pay_size)) {
    return;
  }
  while(sequence_numbers_in_flight() < wnd_size_){
    // If fin has been sent, just return
    if (fin_sent_){
      return;
    }
    pay_size = reader().bytes_buffered();
    if (FIN_sent (pay_size) ) {
      return;
    }
    TCPSenderMessage tcpSenderMessage {.seqno = seqno_, .FIN = fin_, .RST = input_.has_error()};
    string payload_ {};
    if(!sync_) {
      sync_ = true;
      tcpSenderMessage.SYN = true;
      tcpSenderMessage.seqno = isn_;
    } else {
      tcpSenderMessage.SYN = false;
    }
     
    pay_size = min(pay_size, wnd_size_ - sequence_numbers_in_flight() - tcpSenderMessage.SYN);
    pay_size = min(pay_size, TCPConfig::MAX_PAYLOAD_SIZE);
    read(input_.reader(), pay_size, payload_);
    tcpSenderMessage.payload = payload_;
    // If the message is empty, return
    if (tcpSenderMessage.sequence_length() == 0) {
      return;
    }
    if (fin_) {
      fin_sent_ = true;
    }
    transmit(tcpSenderMessage);
    outstanding_segs_.push(tcpSenderMessage);
    seqno_ = seqno_ + tcpSenderMessage.sequence_length();
  }
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
  if(msg.ackno.has_value() && sync_) {
    Wrap32 new_ackno = msg.ackno.value();

    // Ignore old ACKs
    if (unwrap_num(new_ackno) <= unwrap_num(cur_ackno_) 
    && !writer().is_closed() && !fin_sent_) {
      return;
    }
    // Ignore ACKs that are out of range
    if (unwrap_num(new_ackno) > unwrap_num(seqno_)) {
      return;
    }

   if (unwrap_num(new_ackno) > unwrap_num(cur_ackno_)) {
      cur_ackno_ = new_ackno;
   }
    // Remove acked segments
    while(!outstanding_segs_.empty() 
          && unwrap_num(cur_ackno_) >= unwrap_num(outstanding_segs_.front().seqno) + outstanding_segs_.front().sequence_length()) {
            outstanding_segs_.pop();
    }
    if (!outstanding_segs_.empty()) {
      cur_ackno_ = outstanding_segs_.front().seqno;
    }
  }
  
  if (msg.window_size > 0) {
    wnd_size_ = msg.window_size;
    zero_wnd_ = false;
    rtrns_cnt_ = 0;
    RTO_ms_ = initial_RTO_ms_;
    rtrns_timer_ = 0; // When a segment is received, timer should be restarted
  } else {
    // When window size is 0, treat it as a window with size 1
    wnd_size_ = 1;
    zero_wnd_ = true;
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
  
  rtrns_timer_ += ms_since_last_tick;
  
  if(rtrns_timer_ >= RTO_ms_) {
    transmit(outstanding_segs_.front()); // Retransmit the oldest unACKed segment
    if (!zero_wnd_) {
      RTO_ms_ *= 2; // Double the retransmission time
    }
    rtrns_cnt_ ++;
    rtrns_timer_ = 0; 
  }
}