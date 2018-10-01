#ifndef __VDS_CORE_ASYNC_STREAM_H_
#define __VDS_CORE_ASYNC_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <mutex>
#include <memory>


#include "mt_service.h"
#include "not_mutex.h"
#include "stream.h"
#include "vds_debug.h"

namespace vds {
  
  template <typename item_t> 
  class continuous_buffer : public std::enable_shared_from_this<continuous_buffer<item_t>>
  {
  public:
    using item_type = item_t;

    continuous_buffer()
      : second_(0), front_(0), back_(0), eof_(false), eof_readed_(false)
    {
    }

    ~continuous_buffer()
    {
#ifdef _DEBUG
#pragma warning(disable: 4297)
      if (0 != this->second_ || 0 != this->front_ || 0 != this->back_ || this->continue_read_ || this->continue_write_
        || !this->eof_ || !this->eof_readed_) {
        if (!std::current_exception()) {
          throw std::runtime_error("continuous_buffer::~continuous_buffer logic error");
        }
      }
#pragma warning(default: 4297)
#endif//_DEBUG
    }

    std::future<void> write_async(
      const service_provider & sp,
      const item_type * data,
      size_t data_size)
    {
      if (0 == data_size && this->eof_) {
        throw std::runtime_error("Logic error");
      }

      this->in_mutex_.lock();
      this->in_mutex_stack_ = sp.full_name();

      if (0 == data_size) {
        if (this->eof_) {
          this->in_mutex_.unlock();
          throw std::runtime_error("continuous_buffer::write_all_async logic error");
        }
        this->eof_ = true;
        this->eof_stack_ = sp.full_name();

        if (this->continue_read_) {
          std::function<void(void)> f;
          this->continue_read_.swap(f);
#ifdef _DEBUG
          mt_service::async(sp,
            [f]() {
            f();
          });
#else
          mt_service::async(sp, f);
#endif
        }

        this->in_mutex_.unlock();
      }
      else {
        this->in_mutex_.unlock();

        co_await this->write_all(sp, data, data_size);
      }
    }

    std::future<size_t /*readed*/> read_async(const service_provider & sp, item_type * buffer, size_t buffer_size)
    {
      vds_assert(0 != buffer_size);

      if (this->continue_read_) {
        throw std::runtime_error("Logic error 29");
      }

      this->out_mutex_.lock();
      this->out_mutex_stack_ = sp.full_name();

      size_t readed = co_await this->continue_read(sp, buffer, buffer_size);

      this->out_mutex_.unlock();

      co_return readed;
    }

    std::future<const_data_buffer> read_all(const service_provider & sp)
    {
      auto buffer = std::make_shared<std::tuple<resizable_data_buffer, item_type[1024]>>();
      co_return co_await this->read_all(sp, buffer);
    }

  private:
    std::string in_mutex_stack_;
    not_mutex in_mutex_;
    std::string out_mutex_stack_;
    not_mutex out_mutex_;
    std::mutex buffer_mutex_;
    item_type buffer_[4096];
    uint32_t second_;
    uint32_t front_;
    uint32_t back_;
    bool eof_;
    bool eof_readed_;
    std::string eof_stack_;
    //            0    second   front    back   buffer_size
    // to read    [...2...]       [...1...]
    // to write            [..2..]         [...1...]


    std::function<void(void)> continue_write_;
    std::function<void(void)> continue_read_;

    std::future<size_t /*len*/> continue_write(
      const service_provider & sp,
      const item_type * data,
      size_t data_size)
    {
      std::unique_lock<std::mutex> lock(this->buffer_mutex_);
      if (this->back_ < sizeof(this->buffer_) / sizeof(this->buffer_[0])) {
        size_t len = sizeof(this->buffer_) / sizeof(this->buffer_[0]) - this->back_;
        if (len > data_size) {
          len = data_size;
        }

        std::copy(data, data + len, this->buffer_ + this->back_);
        this->back_ += len;

        if (this->continue_read_) {
          std::function<void(void)> f;
          this->continue_read_.swap(f);
          mt_service::async(sp, f);
        }

        lock.unlock();

        co_return len;
      }
      else if (this->second_ < this->front_) {
        auto len = this->front_ - this->second_;
        if (len > data_size) {
          len = data_size;
        }
        std::copy(data, data + len, this->buffer_ + this->second_);
        this->second_ += len;

        if (this->continue_read_) {
          std::function<void(void)> f;
          this->continue_read_.swap(f);
          mt_service::async(sp, f);
        }

        lock.unlock();

        co_return len;
      }

      co_return co_await schedule_write(sp, data, data_size);
    }

    std::future<size_t> schedule_write(const service_provider & sp, const item_type *data, size_t data_size) {
      auto result = std::make_shared<std::promise<size_t>>();
      continue_write_ = [pthis = this->shared_from_this(), sp, result, data, data_size](){
        auto size = pthis->continue_write(sp, data, data_size).get();
        result->set_value(size);
      };
      return result->get_future();
    }

    std::future<void> write_all(
      const service_provider & sp,
      const item_type * data,
      size_t data_size)
    {
      for (;;) {
        size_t len = co_await this->continue_write(sp, data, data_size);

        if (len == data_size) {
          co_return;
        }
        else {
          data += len;
          data_size -= len;
        }
      }
    }

    std::future<size_t /*readed*/> continue_read(
      const service_provider & sp,
      item_type * buffer,
      size_t buffer_size)
    {
      vds_assert(0 != buffer_size);
      std::unique_lock<std::mutex> lock(this->buffer_mutex_);

      if (this->front_ < this->back_) {
        size_t len = this->back_ - this->front_;
        if (len > buffer_size) {
          len = buffer_size;
        }
        std::copy(this->buffer_ + this->front_, this->buffer_ + this->front_ + len, buffer);
        this->front_ += len;

        if (this->front_ == this->back_) {
          this->front_ = 0;
          this->back_ = this->second_;
          this->second_ = 0;
        }

        if (this->continue_write_) {
          std::function<void(void)> f;
          this->continue_write_.swap(f);
          mt_service::async(sp, f);
        }
        vds_assert(0 != len);
        co_return len;
      }
      else if (this->eof_) {
        vds_assert(!this->eof_readed_);
        this->eof_readed_ = true;
        lock.unlock();

        co_return 0;
      }

      auto result = std::make_shared<std::promise<size_t>>();
      this->continue_read_ = [pthis = this->shared_from_this(), sp, result, buffer, buffer_size](){
        try {
          auto size = pthis->continue_read(sp, buffer, buffer_size).get();
          result->set_value(size);
        }
        catch (...) {
          result->set_exception(std::current_exception());
        }
      };

      co_return co_await result->get_future();
    }
    std::future<const_data_buffer> read_all(
      const service_provider & sp,
      const std::shared_ptr<std::tuple<resizable_data_buffer, item_type[1024]>> & buffer)
    {
      for (;;) {
        size_t readed = co_await this->read_async(sp, std::get<1>(*buffer), 1024);

        if (0 == readed) {
          co_return std::get<0>(*buffer).move_data();
        }
        else {
          std::get<0>(*buffer).add(std::get<1>(*buffer), readed);
        }
      }
    }

    void reset()
    {
      if (!this->eof_ || 0 != this->second_ || 0 != this->front_ || 0 != this->back_ || this->continue_read_ || this->continue_write_) {
        throw std::runtime_error("continuous_buffer::reset logic error");
      }

      this->eof_ = false;
      this->eof_readed_ = false;
    }
  };
  
  template <typename item_t>
  class continuous_stream_input_async : public stream_input_async<item_t> {
  public:
    continuous_stream_input_async(const std::shared_ptr<continuous_buffer<item_t>> & buffer)
    : buffer_(buffer) {      
    }

    std::future<size_t> read_async(
      const service_provider &sp,
      uint8_t * buffer,
      size_t len) override {
      return this->buffer_->read_async(sp, buffer, len);
    }


  private:
    std::shared_ptr<continuous_buffer<item_t>> buffer_;
  };
  
  template <typename item_t>
  class continuous_stream_output_async : public stream_output_async<item_t> {
  public:
    continuous_stream_output_async(const std::shared_ptr<continuous_buffer<item_t>> & buffer)
      : buffer_(buffer) {
    }

    std::future<void> write_async(
      const service_provider &sp,
      const uint8_t *data,
      size_t len) override {
      return this->buffer_->write_async(sp, data, len);
    }


  private:
    std::shared_ptr<continuous_buffer<item_t>> buffer_;
  };


  //template <typename item_t>
  //class async_buffer : public stream_output_async<item_t> {
  //public:
  //  using item_type = item_t;

  //  async_buffer(const service_provider &sp)
  //      : stream_output_async<item_t>(new _async_buffer(sp)) {
  //  }

  //  std::future<void> write_value_async(const item_type & data) {
  //    return static_cast<_async_buffer *>(this->impl_.get())->write_value_async(data);
  //  }

  //  std::future<size_t /*readed*/> read_async(item_type *buffer, size_t buffer_size) {
  //    return static_cast<_async_buffer *>(this->impl_.get())->read_async(buffer, buffer_size);
  //  }

  //private:
  //  class _async_buffer : public _stream_async<item_t> {
  //  public:

  //    _async_buffer(const service_provider &sp)
  //        : ready_to_data_(true),
  //          data_(sp) {
  //    }

  //    std::future<void> write_value_async(const item_type &data) {
  //      std::unique_lock<std::mutex> lock(this->data_mutex_);
  //      while (!this->ready_to_data_) {
  //        this->data_barier_.wait(lock);
  //      }

  //      this->ready_to_data_ = false;
  //      return this->data_.write_value_async(data).then(
  //          [pthis = this->shared_from_this()]() {
  //            auto this_ = static_cast<_async_buffer *>(pthis.get());
  //            this_->data_mutex_.lock();
  //            this_->ready_to_data_ = true;
  //            this_->data_barier_.notify_one();
  //            this_->data_mutex_.unlock();
  //          });
  //    }

  //    std::future<void> write_async(const item_type *data, size_t data_size) override {
  //      std::unique_lock<std::mutex> lock(this->data_mutex_);
  //      while (!this->ready_to_data_) {
  //        this->data_barier_.wait_for(lock, std::chrono::microseconds(100));
  //      }

  //      this->ready_to_data_ = false;

  //      return this->data_.write_async(data, data_size).then(
  //          [pthis = this->shared_from_this()]() {
  //            auto this_ = static_cast<_async_buffer *>(pthis.get());
  //            this_->data_mutex_.lock();
  //            this_->ready_to_data_ = true;
  //            this_->data_barier_.notify_one();
  //            this_->data_mutex_.unlock();
  //          });
  //    }

  //    std::future<size_t /*readed*/> read_async(item_type *buffer, size_t buffer_size) {
  //      return this->data_.read_async(buffer, buffer_size);
  //    }

  //  private:
  //    bool ready_to_data_;
  //    std::condition_variable data_barier_;
  //    std::mutex data_mutex_;
  //    continuous_buffer<item_type> data_;
  //  };
  //};
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  //template <typename item_type, typename source_type, typename target_type>
  //class _continue_copy_stream_async : public std::enable_shared_from_this<_continue_copy_stream_async<item_type, source_type, target_type>>
  //{
  //public:
  //  _continue_copy_stream_async(
  //    const service_provider & sp,
  //    const std::shared_ptr<source_type> & source,
  //    const std::shared_ptr<target_type> & target)
  //  : sp_(sp), source_(source), target_(target)
  //  {
  //  }
  //  
  //  static std::future<void> _continue_copy(
  //    const std::shared_ptr<_continue_copy_stream_async> & context) {
  //    for (;;) {
  //      size_t readed = co_await context->source_->read_async(
  //          context->buffer_,
  //          sizeof(context->buffer_) / sizeof(context->buffer_[0]));

  //      co_await context->target_->write_async(context->buffer_, readed);

  //      if (0 == readed) {
  //        co_return;
  //      }
  //    }
  //  }
  //      
  //private:
  //  service_provider sp_;
  //  std::shared_ptr<source_type> source_;
  //  std::shared_ptr<target_type> target_;
  //  item_type buffer_[1024];
  //};
  //
  //
  //template <typename item_type>
  //inline std::future<void> copy_stream(
  //  const service_provider & sp,
  //  const std::shared_ptr<continuous_buffer<item_type>> & source,
  //  const std::shared_ptr<async_buffer<item_type>> & target)
  //{
  //  auto context = std::make_shared<_continue_copy_stream_async<item_type, continuous_buffer<item_type>, async_buffer<item_type>>>(sp, source, target);
  //  return _continue_copy_stream_async<item_type, continuous_buffer<item_type>, async_buffer<item_type>>::_continue_copy(context);
  //}
  //
  //template <typename item_type>
  //inline std::future<void> copy_stream(
  //  const service_provider & sp,
  //  const std::shared_ptr<async_buffer<item_type>> & source,
  //  const std::shared_ptr<continuous_buffer<item_type>> & target)
  //{
  //  auto context = std::make_shared<_continue_copy_stream_async<item_type, async_buffer<item_type>, continuous_buffer<item_type>>>(sp, source, target);
  //  return _continue_copy_stream_async<item_type, async_buffer<item_type>, continuous_buffer<item_type>>::_continue_copy(context);
  //}
  //
  //template <typename item_type>
  //inline std::future<void> copy_stream(
  //  const service_provider & sp,
  //  const std::shared_ptr<continuous_buffer<item_type>> & source,
  //  const std::shared_ptr<continuous_buffer<item_type>> & target)
  //{
  //  auto context = std::make_shared<_continue_copy_stream_async<item_type, continuous_buffer<item_type>, continuous_buffer<item_type>>>(sp, source, target);
  //  return _continue_copy_stream_async<item_type, continuous_buffer<item_type>, continuous_buffer<item_type>>::_continue_copy(context);
  //}
  //
  //template <typename item_type>
  //inline std::future<void> copy_stream(
  //  const service_provider & sp,
  //  const std::shared_ptr<continuous_buffer<item_type>> & source,
  //  const std::shared_ptr<stream_output_async<item_type>> & target)
  //{
  //  auto context = std::make_shared<_continue_copy_stream_async<item_type, continuous_buffer<item_type>, stream_output_async<item_type>>>(sp, source, target);
  //  return _continue_copy_stream_async<item_type, continuous_buffer<item_type>, stream_output_async<item_type>>::_continue_copy(context);
  //}
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  //template <typename item_type, typename source_type, typename target_type>
  //class _continue_copy_stream : public std::enable_shared_from_this<_continue_copy_stream<item_type, source_type, target_type>>
  //{
  //public:
  //  _continue_copy_stream(
  //    const service_provider & sp,
  //    const std::shared_ptr<source_type> & source,
  //    const std::shared_ptr<target_type> & target)
  //    : sp_(sp), source_(source), target_(target)
  //  {
  //  }

  //  static std::future<void> _continue_copy(
  //    const std::shared_ptr<_continue_copy_stream> & context) {
  //    for (;;) {
  //      size_t readed = co_await
  //      context->source_->read_async(
  //          context->buffer_,
  //          sizeof(context->buffer_) / sizeof(context->buffer_[0]));

  //      co_await
  //      context->target_->write(context->buffer_, readed);
  //      if (0 == readed) {
  //        co_return;
  //      }
  //    }
  //  }

  //private:
  //  service_provider sp_;
  //  std::shared_ptr<source_type> source_;
  //  std::shared_ptr<target_type> target_;
  //  item_type buffer_[1024];
  //};

  //template <typename item_type>
  //inline std::future<void> copy_stream(
  //  const service_provider & sp,
  //  const std::shared_ptr<continuous_buffer<item_type>> & source,
  //  const std::shared_ptr<stream<item_type>> & target)
  //{
  //  auto context = std::make_shared<_continue_copy_stream<item_type, continuous_buffer<item_type>, stream<item_type>>>(sp, source, target);
  //  return _continue_copy_stream<item_type, continuous_buffer<item_type>, stream<item_type>>::_continue_copy(context);
  //}

  //template <typename item_type>
  //inline std::future<void> copy_stream(
  //  const service_provider & sp,
  //  const std::shared_ptr<async_buffer<item_type>> & source,
  //  const std::shared_ptr<stream<item_type>> & target)
  //{
  //  auto context = std::make_shared<_continue_copy_stream<item_type, async_buffer<item_type>, stream<item_type>>>(sp, source, target);
  //  return _continue_copy_stream<item_type, async_buffer<item_type>, stream<item_type>>::_continue_copy(context);
  //}

}

#endif // __VDS_CORE_ASYNC_STREAM_H_
 
