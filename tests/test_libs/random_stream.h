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
class random_stream : public vds::stream<item_type> {
public:
  random_stream(const vds::stream<item_type> &target)
      : vds::stream<item_type>(new _random_stream(target)) {
  }

private:
  class _random_stream : public vds::_stream<item_type> {
  public:
    _random_stream(const vds::stream<item_type> &target)
        : target_(target) {
    }

    void write(const item_type *data, size_t len) override {
      if (0 == len) {
        this->target_.write(data, len);
      } else {
        while (0 < len) {
          size_t n = (size_t) std::rand() % len;
          if (n < 1) {
            n = 1;
          }
          if (len < n) {
            n = len;
          }

          this->target_.write(data, len);

          data += n;
          len -= n;
        }
      }
    }

  private:
    vds::stream<item_type> target_;
  };
};

template<typename item_type>
class random_stream_async : public vds::stream_async<item_type> {
public:
  random_stream_async(vds::stream_async<item_type> &target)
      : vds::stream_async<item_type>(new _random_stream_async(target)) {
  }

private:
  class _random_stream_async : public vds::_stream_async<item_type> {
  public:
    _random_stream_async(vds::stream_async<item_type> &target)
        : target_(target) {
    }

    std::future<void> write_async(const item_type *data, size_t len) override {
      if (0 == len) {
        return this->target_.write_async(data, len);
      } else {
        size_t n = (size_t) std::rand() % len;
        if (n < 1) {
          n = 1;
        }
        if (len < n) {
          n = len;
        }

        if (n == len) {
          return this->target_.write_async(data, n);
        }

        return
            this->target_.write_async(data, n)
                .then([pthis = this->shared_from_this(), p = data + n, l = len - n]() {
                  return pthis->write_async(p, l);
                });
      }
    }

  private:
    vds::stream_async<item_type> &target_;
  };
};

#endif // __TEST_VDS_LIBS__RANDOM_READER_H_
