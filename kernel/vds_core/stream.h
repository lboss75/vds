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
  class stream_output_async : public std::enable_shared_from_this<stream_output_async<item_type>> {
  public:

    virtual ~stream_output_async() {}

    virtual vds::async_task<void> write_async(
        const item_type *data,
        size_t len) = 0;
  };


  class file_stream_output_async : public stream_output_async<uint8_t> {
  public:
    file_stream_output_async(const filename & fn, const file::file_mode mode);
    explicit file_stream_output_async(file * f);
    ~file_stream_output_async();

    vds::async_task<void> write_async(
      const uint8_t *data,
      size_t len) override;

  private:
    file own_file_;
    file * f_;
  };

  class null_stream_output_async : public stream_output_async<uint8_t> {
  public:
    vds::async_task<void> write_async(
      const uint8_t *data,
      size_t len) override;
  };
  ///////////////////////////////////////////////////////////

  template <typename item_type>
  class stream_input_async : public std::enable_shared_from_this<stream_input_async<item_type>> {
  public:
    virtual ~stream_input_async() {}

    virtual vds::async_task<size_t> read_async(      
      item_type * buffer,
      size_t len) = 0;

    vds::async_task<const_data_buffer> read_all() {
      auto result = std::make_shared<resizable_data_buffer>();
      for(;;) {
        result->resize_data(result->size() + 1024);
        auto readed = co_await this->read_async(const_cast<uint8_t *>(result->data() + result->size()), 1024);
        if(readed == 0) {
          co_return result->move_data();
        }

        result->apply_size(readed);
      }
    }
  };

  ///////////////////////////////////////////////////////////

  class buffer_stream_input_async : public stream_input_async<uint8_t> {
  public:
    buffer_stream_input_async(const const_data_buffer & data)
      : data_(data), readed_(0) {
    }

    buffer_stream_input_async(const_data_buffer && data)
    : data_(std::move(data)), readed_(0) {      
    }

    vds::async_task<size_t> read_async(      
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

  class file_stream_input_async : public stream_input_async<uint8_t> {
  public:
    file_stream_input_async(const filename & fn)
      : f_(fn, file::file_mode::open_read),
    processed_(0),
    readed_(0),
    eof_(0) {
    }

    vds::async_task<size_t> read_async(
      
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
   
  ///////////////////////////////////////////////////////////
  template <typename item_type>
  class collect_data : public stream_output_async<item_type>
  {
  public:
    collect_data() {
    }

    vds::async_task<void> write_async(
      const item_type *data,
      size_t len) override {
        this->data_.add(data, len);
        co_return;
      }

      const_data_buffer move_data() {
        return this->data_.move_data();
      }

    private:
      resizable_data_buffer data_;
  };
}

#endif//__VDS_CORE_STREAM_H_
