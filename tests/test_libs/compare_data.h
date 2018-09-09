/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__COMPARE_DATA_H_
#define __TEST_VDS_LIBS__COMPARE_DATA_H_

#include "targetver.h"
#include <memory>
#include "stream.h"

template <typename item_type>
class compare_data : public vds::stream<item_type> {
public:
  compare_data(
      const item_type *data,
      size_t len)
      : vds::stream<item_type>(new _compare_data(data, len)) {

  }

private:
  class _compare_data : public vds::_stream<item_type> {
  public:
    _compare_data(
        const item_type *data,
        size_t len)
        : data_(data),
          len_(len) {
    }

    void write(
        const item_type *data,
        size_t len) override {
      if (0 == len) {
        if (0 != this->len_) {
          throw std::runtime_error("Unexpected end of stream while comparing data");
        }

        return;
      }

      if (this->len_ < len) {
        throw std::runtime_error("Unexpected data while comparing data");
      }

      if (0 != memcmp(this->data_, data, len)) {
        throw std::runtime_error("Compare data error");
      }

      this->data_ += len;
      this->len_ -= len;
    }

  private:
    const item_type *data_;
    size_t len_;
  };
};

template <typename item_type>
class compare_data_async : public vds::stream_async<item_type>
{
public:
  compare_data_async(
      const item_type *data,
      size_t len)
      : vds::stream<item_type>(new _compare_data(data, len)) {

  }
private:

  class _compare_data : public vds::stream_async<item_type>
  {
  public:

    _compare_data(
        const item_type * data,
        size_t len)
        : data_(data),
          len_(len)
    {
    }

    std::future<void> write_async(
        const item_type * data,
        size_t len) override
    {
      if (0 == len) {
        if (0 != this->len_) {
          return std::future<void>(std::make_shared<std::runtime_error>("Unexpected end of stream while comparing data"));
        }

        return std::future<void>::empty();
      }

      if (this->len_ < len) {
        return std::future<void>(std::make_shared<std::runtime_error>("Unexpected data while comparing data"));
      }

      if (0 != memcmp(this->data_, data, len)) {
        return std::future<void>(std::make_shared<std::runtime_error>("Compare data error"));
      }

      this->data_ += len;
      this->len_ -= len;

      return std::future<void>::empty();
    }

  private:
    const item_type * data_;
    size_t len_;
  };
};


#endif // __TEST_VDS_LIBS__COMPARE_DATA_H_
