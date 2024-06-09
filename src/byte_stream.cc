#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return {input_end_};
}

void Writer::push( string data )
{
  // Your code here.
  (void)data;
  uint64_t valid_size = (available_capacity() >= data.size() 
  ? data.size() : available_capacity());
  buffer_ += data.substr(0, valid_size);
  used_cap_ += valid_size;
  bytes_written_ += valid_size;
}

void Writer::close()
{
  // Your code here.
  input_end_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return {capacity_ - used_cap_};
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return {bytes_written_};
}

bool Reader::is_finished() const
{
  // Your code here.
  return {input_end_ && (used_cap_ == 0)};
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return {bytes_read_};
}

std::string Reader::peek() const
{
  // Your code here.
  return {buffer_};
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  (void)len;
  uint64_t popped_size = (used_cap_ >= len 
  ? len : used_cap_); 
  buffer_.erase(0, popped_size);
  used_cap_ -= popped_size;
  bytes_read_ += popped_size;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return {used_cap_};
}
