/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__RANDOM_BUFFER_H_
#define __TEST_VDS_LIBS__RANDOM_BUFFER_H_

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

#endif // __TEST_VDS_LIBS__RANDOM_BUFFER_H_
