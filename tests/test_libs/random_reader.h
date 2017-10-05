/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__RANDOM_READER_H_
#define __TEST_VDS_LIBS__RANDOM_READER_H_

#include "targetver.h"
#include "types.h"

template<typename item_type>
class random_reader
{
public:
  random_reader(
    const item_type * data,
    size_t len)
  : data_(data),
    len_(len)
  {
  }


    size_t sync_get_data(
      const item_type * data,
      size_t len)
    {
      for (;;) {
        size_t n = (size_t)std::rand() % len;
        if (n < 1 || n > len) {
          continue;
        }

        if (n > this->len_) {
          n = this->len_;
        }
        
        if(0 == n){
          return 0;
        }

        std::copy(this->data_, this->data_ + n, data);

        this->data_ += n;
        this->len_ -= n;

        return n;
      }
    }

  private:
    const item_type * data_;
    size_t len_;
  };


#endif // __TEST_VDS_LIBS__RANDOM_READER_H_
