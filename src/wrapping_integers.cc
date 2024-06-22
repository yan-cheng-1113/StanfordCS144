#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32 {zero_point + n};
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t offset = raw_value_ - zero_point.raw_value_;
  uint64_t unit = 1ul << 32;

  auto n = checkpoint / unit;
  checkpoint %= unit;

  if ((checkpoint > offset) && (checkpoint - offset > unit / 2)) {
    return offset + (n + 1) * unit;
  }
  else if ((offset > checkpoint) && (offset - checkpoint > unit / 2) && (n > 0)) {
    return offset + (n - 1) * unit;
  }
  return offset + n * unit;
}
