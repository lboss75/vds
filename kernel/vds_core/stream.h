#ifndef __VDS_CORE_STREAM_H_
#define __VDS_CORE_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <vector>
#include "async_task.h"

namespace vds {
  template <typename item_type>
  class _stream_async : public std::enable_shared_from_this<_stream_async<item_type>> {
  public:
    ~_stream_async() {}
    virtual async_task<> write_async(
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

    async_task<> write_async(
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

    const item_type * data() const
    {
      return static_cast<const _collect_data *>(this->impl_.get())->data();
    }
    
    size_t size() const
    {
      return static_cast<const _collect_data *>(this->impl_.get())->size();
    }

  protected:
    class _collect_data : public _stream<item_type> {
    public:
      void write(
          const item_type *data,
          size_t len) override {
        for (size_t i = 0; i < len; ++i) {
          this->data_.push_back(data[i]);
        }
      }

      const item_type *data() const {
        return this->data_.data();
      }

      size_t size() const {
        return this->data_.size();
      }

    private:
      std::vector<item_type> data_;
    };
  };
}

#endif//__VDS_CORE_STREAM_H_
