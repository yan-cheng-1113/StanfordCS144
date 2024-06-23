#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  RST_ = message.RST;
  if (!isn_.has_value()){
    if (message.SYN) {
      isn_ = message.seqno;
      ackno_ = message.seqno + message.sequence_length();
    }
    if(message.FIN) {
      reader().writer().close();
    }
    if(message.RST){
      reader().writer().set_error();
    }
  } 
  else {
    uint64_t abs_seqno = message.seqno.unwrap(isn_.value(), reassembler_.first_unassembled());
    reassembler_.insert(abs_seqno - 1, message.payload, message.FIN);
    if (writer().is_closed()){
      ackno_ = message.seqno.wrap(reassembler_.first_unassembled() + 2, isn_.value());
    }
    else{
      ackno_ = message.seqno.wrap(reassembler_.first_unassembled() + 1, isn_.value());
    }
    wnd_size_ = writer().available_capacity();
    if(message.RST){
      reader().writer().set_error();
    }
  } 
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here. 
  // wnd_size_ = writer().available_capacity();
  TCPReceiverMessage tcpReceiverMessage_ {.ackno = ackno_,
    .window_size = wnd_size_, 
    .RST = RST_};
  return {tcpReceiverMessage_};
}
