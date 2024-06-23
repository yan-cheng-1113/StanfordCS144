#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if(message.RST){
    reader().writer().set_error();
  } 

  if (!isn_.has_value()){
    if (message.SYN) {
      isn_ = message.seqno;
      ackno_ = message.seqno + message.sequence_length();
      uint64_t abs_seqno = message.seqno.unwrap(isn_.value(), reassembler_.first_unassembled());
      reassembler_.insert(abs_seqno, message.payload, message.FIN);
    }
  } 
  else {
    uint64_t abs_seqno = message.seqno.unwrap(isn_.value(), reassembler_.first_unassembled());
    reassembler_.insert(abs_seqno - 1, message.payload, message.FIN); // Subtract one to get stream index
    if (writer().is_closed()){
      // The case with SYN and FIN
      ackno_ = message.seqno.wrap(reassembler_.first_unassembled() + 2, isn_.value());
    }
    else{
      // The case with only SYN
      ackno_ = message.seqno.wrap(reassembler_.first_unassembled() + 1, isn_.value());
    }
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here. 
  uint16_t wnd_size_ = static_cast<uint16_t> ((reassembler_.writer().available_capacity()) <= UINT16_MAX ? 
        (reassembler_.writer().available_capacity()) : UINT16_MAX); 
  
  bool RST_ = reassembler_.reader().has_error();

  TCPReceiverMessage tcpReceiverMessage_ {.ackno = ackno_,
                                          .window_size = wnd_size_, 
                                          .RST = RST_};
  return {tcpReceiverMessage_};
}
