#include "random_buffer.h"

random_buffer::random_buffer(size_t min_size, size_t max_size)
{
  size_t len;
  do
  {
    len = (min_size + (size_t)std::rand()) % max_size;
  } while(len < min_size || len > max_size);
  
  for(size_t i = 0; i < len; ++i) {
    this->data_.push_back((uint8_t)std::rand());
  }
}
