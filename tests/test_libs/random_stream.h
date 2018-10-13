/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__RANDOM_READER_H_
#define __TEST_VDS_LIBS__RANDOM_READER_H_

#include "targetver.h"
#include "types.h"
#include "stream.h"

template<typename item_type>
class random_stream : public vds::stream_output_async<item_type> {
public:
  random_stream(const std::shared_ptr<vds::stream_output_async<item_type>> &target)
    : target_(target) {
  }

  vds::async_task<void> write_async( const item_type *data, size_t len) override {
    for (;;) {
      if (0 == len) {
        co_await this->target_->write_async(data, len);
        co_return;
      }
      else {
        size_t n = (size_t)std::rand() % len;
        if (n < 1) {
          n = 1;
        }
        if (len < n) {
          n = len;
        }

        if (n == len) {
          co_await this->target_->write_async(data, n);
          co_return;
        }
        else {
          co_await this->target_->write_async(data, n);

          data += n;
          len -= n;
        }
      }
    }
  }

private:
  std::shared_ptr<vds::stream_output_async<item_type>> target_;
};


#endif // __TEST_VDS_LIBS__RANDOM_READER_H_
