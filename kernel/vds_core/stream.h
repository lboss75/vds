#ifndef __VDS_CORE_STREAM_H_
#define __VDS_CORE_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"
#include "resizable_data_buffer.h"
#include "file.h"

namespace vds {
  template <typename item_type>
  class _stream_async : public std::enable_shared_from_this<_stream_async<item_type>> {
  public:
    ~_stream_async() {}
    virtual vds::async_task<void> write_async(
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

    vds::async_task<void> write_async(
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
  ///////////////////////////////////////////////////////////

  template <typename item_type>
  class input_stream_async : public std::enable_shared_from_this<input_stream_async<item_type>> {
  public:
    virtual vds::async_task<size_t> read_async(
      const service_provider &sp,
      item_type * buffer,
      size_t len) = 0;

    vds::async_task<const_data_buffer> read_all(const service_provider &sp) {
      auto result = std::make_shared<resizable_data_buffer>();
      for(;;) {
        result->resize_data(result->size() + 1024);
        auto readed = co_await this->read_async(sp, const_cast<uint8_t *>(result->data() + result->size()), 1024);
        if(readed == 0) {
          co_return result->move_data();
        }

        result->apply_size(readed);
      }
    }
  };

  class buffer_input_stream_async : public input_stream_async<uint8_t> {
  public:
    buffer_input_stream_async(const const_data_buffer & data)
      : data_(data), readed_(0) {
    }

    buffer_input_stream_async(const_data_buffer && data)
    : data_(std::move(data)), readed_(0) {      
    }

    async_task<size_t> read_async(
      const service_provider &sp,
      uint8_t * buffer,
      size_t len) override {
      if(this->readed_ > this->data_.size()) {
        co_return 0;
      }

      if(len > this->data_.size() - this->readed_) {
        len = this->data_.size() - this->readed_;
      }

      memcpy(buffer, this->data_.data() + this->readed_, len);
      this->readed_ += len;

      co_return len;
    }

  private:
    const_data_buffer data_;
    size_t readed_;
  };

  class file_input_stream_async : public input_stream_async<uint8_t> {
  public:
    file_input_stream_async(const filename & fn)
      : f_(fn, file::file_mode::open_read),
    processed_(0),
    readed_(0),
    eof_(0) {
    }

    async_task<size_t> read_async(
      const service_provider &sp,
      uint8_t * buffer,
      size_t len) override {
      for (;;) {
        if (this->readed_ > this->processed_) {
          if (len > this->readed_ - this->processed_) {
            len = this->readed_ - this->processed_;
          }

          memcpy(buffer, this->buffer_ + this->processed_, len);
          this->processed_ += len;
          co_return len;
        }
        if (this->eof_) {
          co_return 0;
        }
        this->processed_ = 0;
        this->readed_ = this->f_.read(this->buffer_, sizeof(this->buffer_));
        if (0 == this->readed_) {
          this->eof_ = true;
          co_return 0;
        }
      }
    }

  private:
    file f_;

    uint8_t buffer_[1024];
    size_t processed_;
    size_t readed_;
    bool eof_;
  };

  ///////////////////////////////////////////////////////////




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
