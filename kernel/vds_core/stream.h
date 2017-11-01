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
  class stream_async
  {
  public:
    async_task<> write_async(
        const service_provider &sp,
        const item_type *data,
        size_t len)
    {
      return  this->impl_->write_async(sp, data, len);
    }

  protected:
    class _stream_async {
    public:
      ~_stream_async() {}
      virtual async_task<> write_async(
          const service_provider &sp,
          const item_type *data,
          size_t len) = 0;
    };

    std::shared_ptr<_stream_async> impl_;

    stream_async(_stream_async * impl)
        : impl_(impl)
    {
    }
  };

  template <typename item_type>
  class stream
  {
  public:
    void write(
        const service_provider &sp,
        const item_type *data,
        size_t len){
      this->impl_->write(sp, data, len);
    }

  protected:
    class _stream {
    public:
      virtual ~_stream() {}

      virtual void write(
          const service_provider &sp,
          const item_type *data,
          size_t len) = 0;
    };

    std::shared_ptr<_stream> impl_;

    stream(_stream * impl)
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
      return static_cast<const collect_data *>(this->impl_.get())->data();
    }
    
    size_t size() const
    {
      return static_cast<const collect_data *>(this->impl_.get())->size();
    }

  protected:
    class _collect_data : public _stream {
    public:
      void write(
          const service_provider &sp,
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
