/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__COMPARE_DATA_H_
#define __TEST_VDS_LIBS__COMPARE_DATA_H_

#include "targetver.h"
#include <memory>

template <typename item_type>
class compare_data
{
public:
  compare_data(
    const item_type * data,
    size_t len)
    : data_(data),
    len_(len)
  {
  }

  std::shared_ptr<std::exception> update(
    const item_type * data,
    size_t len)
    {
      if (0 == len) {
        if (0 != this->len_) {
          this->in_error_ = true;
          return std::make_shared<std::runtime_error>("Unexpected end of stream while comparing data");
        }
        
        return std::shared_ptr<std::exception>();
      }

      if (this->len_ < len) {
        this->in_error_ = true;
        return std::make_shared<std::runtime_error>("Unexpected data while comparing data");
      }

      if (0 != memcmp(this->data_, this->input_buffer(), this->input_buffer_size())) {
        this->in_error_ = true;
        return std::make_shared<std::runtime_error>("Compare data error");
      }

      this->data_ += this->input_buffer_size();
      this->len_ -= this->input_buffer_size();

      return std::shared_ptr<std::exception>();
    }

  private:
    const item_type * data_;
    size_t len_;
  };


#endif // __TEST_VDS_LIBS__COMPARE_DATA_H_
