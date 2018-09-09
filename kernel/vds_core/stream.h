#ifndef __VDS_CORE_STREAM_H_
#define __VDS_CORE_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <vector>
#include <future>

#include "resizable_data_buffer.h"

namespace vds {
  template <typename item_type>
  class _stream_async : public std::enable_shared_from_this<_stream_async<item_type>> {
  public:
    ~_stream_async() {}
    virtual std::future<void> write_async(
        const item_type *data,
        size_t len) = 0;
  };


  template <typename item_type>
  class stream_async
  {
  public:
    stream_async(const stream_async & origin)
    : impl_(origin.impl_)
    {
    }

    std::future<void> write_async(
        const item_type *data,
        size_t len)
    {
      return  this->impl_->write_async(data, len);
    }

    operator bool () const {
      return this->impl_.operator bool();
    }

  protected:

    std::shared_ptr<_stream_async<item_type>> impl_;

    stream_async(_stream_async<item_type> * impl)
        : impl_(impl)
    {
    }
  };

  template <typename item_type>
  class _stream : public std::enable_shared_from_this<_stream<item_type>> {
  public:
    virtual ~_stream() {}

    virtual void write(
        const item_type *data,
        size_t len) = 0;
  };

  template <typename item_type>
  class stream
  {
  public:
    void write(
        const item_type *data,
        size_t len){
      this->impl_->write(data, len);
    }

  protected:
    std::shared_ptr<_stream<item_type>> impl_;

    stream(_stream<item_type> * impl)
    : impl_(impl)
    {
    }
  };
  
  template <typename item_type>
  class collect_data : public stream<item_type>
  {
  public:
    collect_data()
    : stream<item_type>(new _collect_data())
    {
    }

    const_data_buffer move_data() 
    {
      return static_cast<_collect_data *>(this->impl_.get())->move_data();
    }

  protected:
    class _collect_data : public _stream<item_type> {
    public:
      void write(
          const item_type *data,
          size_t len) override {
        this->data_.add(data, len);
      }

      const_data_buffer move_data() {
        return this->data_.move_data();
      }
    private:
      resizable_data_buffer data_;
    };
  };
}

#endif//__VDS_CORE_STREAM_H_
