#ifndef RANDOM_BUFFER_H
#define RANDOM_BUFFER_H

#include "targetver.h"
#include "types.h"
#include <vector>

class random_buffer
{
public:
  random_buffer(size_t min_size = 1024, size_t max_size = 1024 * 1024);
  
  const uint8_t * data() const { return this->data_.data(); }
  size_t size() const { return this->data_.size(); }
  
public:
  std::vector<uint8_t> data_;
};

#endif // RANDOM_BUFFER_H
