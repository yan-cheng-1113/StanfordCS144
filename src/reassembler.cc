#include "reassembler.hh"


using namespace std;

void Reassembler:: merge_insert(Segment segment)
{
  auto iter = rbuffer_.lower_bound(segment);
  while (iter != rbuffer_.end()){
    // Case when the incoming segment can be concatenated with an exisiting segment
    if (segment.first + segment.second.size() == iter->first){
      segment.second += iter->second;
      unassembled_size_ -= iter->second.size();
      rbuffer_.erase(iter);
      iter = rbuffer_.lower_bound(segment);
    }
    // Case when the incoming segment intersects an existing segment
    else if (segment.first + segment.second.size() > iter->first) {
      // Case when the incoming segment completely covers an existing segment, should remove the existing segment
      if(segment.first + segment.second.size() >= iter->first + iter->second.size()){
        unassembled_size_ -= iter->second.size(); 
        rbuffer_.erase(iter);
        iter = rbuffer_.lower_bound(segment); 
        continue;
      }
      uint64_t len = iter->first + iter->second.size() - segment.first - segment.second.size();
      uint64_t starting_index = iter->second.size() - len; 
      unassembled_size_ -= iter->second.size();
      segment.second += iter->second.substr(starting_index, len);
      rbuffer_.erase(iter);
      iter = rbuffer_.lower_bound(segment); 
    }
    else{
      break;
    }
  }

  if(!rbuffer_.empty()){
    while(iter != rbuffer_.begin() && !rbuffer_.empty()){
      iter--;
      // Case when the incoming segment can be concatenated with an exisiting segment
      if (iter->first + iter->second.size() == segment.first){
        segment.first = iter->first;
        segment.second = iter->second + segment.second;
        unassembled_size_ -= iter->second.size();
        rbuffer_.erase(iter);
        iter = rbuffer_.lower_bound(segment);
        
      }
      // Case when the incoming segment intersects an existing segment
      else if (iter->first + iter->second.size() > segment.first){
        // Case when the incoming segment is covered by an existing segment, should discard the incoming segment
        if(iter->first + iter->second.size() >= segment.first + segment.second.size()){
          return;
        }
        uint64_t len = segment.first - iter->first;
        unassembled_size_ -= iter->second.size();
        segment.first = iter->first;
        segment.second = iter->second.substr(0, len) + segment.second;
        rbuffer_.erase(iter);
        iter = rbuffer_.lower_bound(segment);
      }
      else{
        break;
      }
    }
  }
  
  // Insert
  if(rbuffer_.find(segment) != rbuffer_.end()){
    return;
  }
  rbuffer_.insert(segment);
  unassembled_size_ += segment.second.size();
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  uint64_t windex = first_index;
  bool trimmed {false};
  if (writer().available_capacity() == 0 || unassembled_size_ >= writer().available_capacity()) {
    return;
  }
  if (first_index >= next_index_ + writer().available_capacity()){
    return;
  }
  if (!(data.empty()) && first_index + data.size() <= next_index_){
    return;
  }

  if (first_index < next_index_ && next_index_ <= first_index + data.size()){
    uint64_t wsize = (first_index + data.size() - next_index_ <= writer().available_capacity() - unassembled_size_
                      ? first_index + data.size() - next_index_ : writer().available_capacity());
    
    if (first_index + data.size() - next_index_ > writer().available_capacity()) {
      trimmed = true;
    }
    data = data.substr(next_index_ - first_index, wsize);
    windex = next_index_;
  }

  if(first_index >= next_index_){
    uint64_t wsize = (first_index + data.size() <= next_index_ + writer().available_capacity()
                      ? data.size() : writer().available_capacity() - first_index + next_index_);

    if (wsize < data.size()) {
      trimmed = true;
    }
    data = data.substr(0, wsize);
  }

  if(is_last_substring && !trimmed){
    eof_flag_ = true;
  }

  Segment segment {windex, data};
  merge_insert(segment);
  if (windex == next_index_){ 
    output_.writer().push(rbuffer_.begin()->second);
    next_index_ += rbuffer_.begin()->second.size();
    unassembled_size_ -= rbuffer_.begin()->second.size();
    rbuffer_.erase(rbuffer_.begin());
      
    if(eof_flag_ && rbuffer_.empty()){
      output_.writer().close();
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return {unassembled_size_};
}
