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
  class continuous_buffer : public stream_async<item_t>
  {
  public:
    using item_type = item_t;
    
    continuous_buffer(const service_provider & sp)
    : stream_async<item_t>(new _continuous_buffer(sp))
    {
    }

    continuous_buffer(const continuous_buffer & origin)
        : stream_async<item_t>(origin)
    {
    }

    vds::async_task<void> write_value_async(const item_type & data)
    {
      auto p = std::make_shared<item_type>(data);
      return this->write_async(p.get(), 1).then([p]() {});
    }
    
    vds::async_task<size_t /*readed*/> read_async(item_type * buffer, size_t buffer_size)
    {
      return static_cast<_continuous_buffer *>(this->impl_.get())->read_async(buffer, buffer_size);
    }

    vds::async_task<const_data_buffer> read_all()
    {
      auto buffer = std::make_shared<std::tuple<resizable_data_buffer, item_type [1024]>>();
      return static_cast<_continuous_buffer *>(this->impl_.get())->read_all(buffer);
    }

    void reset()
    {
      return static_cast<_continuous_buffer *>(this->impl_.get())->reset();
    }

    continuous_buffer & operator = (continuous_buffer && origin)
    {
      this->impl_ = std::move(origin.impl_);
      return  *this;
    }
    
  private:
    class _continuous_buffer : public _stream_async<item_t>
    {
    public:

      _continuous_buffer(const service_provider & sp)
        : sp_(sp), second_(0), front_(0), back_(0), eof_(false), eof_readed_(false)
      {
      }

      ~_continuous_buffer()
      {
  #ifdef _DEBUG
  #pragma warning(disable: 4297)
        if (0 != this->second_ || 0 != this->front_ || 0 != this->back_ || this->continue_read_ || this->continue_write_ 
          || !this->eof_ || !this->eof_readed_) {
          if(!std::current_exception()){
            throw std::runtime_error("continuous_buffer::~continuous_buffer logic error");
          }
        }
  #pragma warning(default: 4297)
  #endif//_DEBUG
      }

      vds::async_task<void> write_async(
        const item_type * data,
        size_t data_size) override
      {
        if (0 == data_size && this->eof_) {
          throw std::runtime_error("Logic error");
        }

        this->in_mutex_.lock();
        this->in_mutex_stack_ = this->sp_.full_name();

        if (0 == data_size) {
          if (this->eof_) {
            this->in_mutex_.unlock();
            throw std::runtime_error("continuous_buffer::write_all_async logic error");
          }
          this->eof_ = true;
          this->eof_stack_ = this->sp_.full_name();

          if (this->continue_read_) {
            std::function<void(void)> f;
            this->continue_read_.swap(f);
#ifdef _DEBUG
            mt_service::async(this->sp_,
              [f]() {
              f();
            });
#else
            mt_service::async(this_->sp_, f);
#endif
          }

          this->in_mutex_.unlock();
        }
        else {
          this->in_mutex_.unlock();

          co_await this->write_all(data, data_size);
        }
      }

      vds::async_task<size_t /*readed*/> read_async(item_type * buffer, size_t buffer_size)
      {
        vds_assert(0 != buffer_size);

        if (this->continue_read_) {
          throw std::runtime_error("Logic error 29");
        }
        
        this->out_mutex_.lock();
        this->out_mutex_stack_ = this->sp_.full_name();

        size_t readed = co_await this->continue_read(buffer, buffer_size);

        this->out_mutex_.unlock();

        co_return readed;
      }

      vds::async_task<const_data_buffer> read_all(
          const std::shared_ptr<std::tuple<resizable_data_buffer, item_type [1024]>> & buffer)
      {
        for (;;) {
          size_t readed = co_await this->read_async(std::get<1>(*buffer), 1024);

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

    private:
      service_provider sp_;
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

      async_task<size_t /*len*/> continue_write(
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
        
          if(this->continue_read_){
            std::function<void(void)> f;
            this->continue_read_.swap(f);
            mt_service::async(this->sp_, f);
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
        
          if(this->continue_read_){
            std::function<void(void)> f;
            this->continue_read_.swap(f);
            mt_service::async(this->sp_, f);
          }

          lock.unlock();

          co_return len;
        }
        else {
          auto result = std::make_shared<async_result<size_t>>();
          this->continue_write_ = [pthis = this->shared_from_this(), result, data, data_size](){
            auto size = static_cast<_continuous_buffer *>(pthis.get())->continue_write(data, data_size).get();
            result->set_value(size);
          };
          co_return co_await result->get_future();
        }
      }
      
      async_task<void> write_all(
        const item_type * data,
        size_t data_size)
      {
        for (;;) {
          size_t len = co_await this->continue_write(data, data_size);

          if (len == data_size) {
            co_return;
          }
          else {
            data += len;
            data_size -= len;
          }
        }
      }

      async_task<size_t /*readed*/> continue_read(
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
          
          if(this->front_ == this->back_){
            this->front_ = 0;
            this->back_ = this->second_;
            this->second_ = 0;
          }          
          
          if(this->continue_write_){
            std::function<void(void)> f;
            this->continue_write_.swap(f);
            mt_service::async(this->sp_, f);
          }
          vds_assert(0 != len);
          co_return len;
        }
        else if(this->eof_){
          vds_assert(!this->eof_readed_);
          this->eof_readed_ = true;
          lock.unlock();

          co_return 0;
        }
        else {
          auto result = std::make_shared<async_result<size_t>>();
          this->continue_read_ = [pthis = this->shared_from_this(), result, buffer, buffer_size](){
            try {
              auto size = static_cast<_continuous_buffer *>(pthis.get())->continue_read(buffer, buffer_size).get();
              result->set_value(size);
            }
            catch(...) {
              result->set_exception(std::current_exception());
            }
          };

          co_return co_await result->get_future();
        }
      }
    };
  };
  
  template <typename item_t>
  class async_buffer : public stream_async<item_t> {
  public:
    using item_type = item_t;

    async_buffer(const service_provider &sp)
        : stream_async<item_t>(new _async_buffer(sp)) {
    }

    vds::async_task<void> write_value_async(const item_type & data) {
      return static_cast<_async_buffer *>(this->impl_.get())->write_value_async(data);
    }

    vds::async_task<size_t /*readed*/> read_async(item_type *buffer, size_t buffer_size) {
      return static_cast<_async_buffer *>(this->impl_.get())->read_async(buffer, buffer_size);
    }

  private:
    class _async_buffer : public _stream_async<item_t> {
    public:

      _async_buffer(const service_provider &sp)
          : ready_to_data_(true),
            data_(sp) {
      }

      vds::async_task<void> write_value_async(const item_type &data) {
        std::unique_lock<std::mutex> lock(this->data_mutex_);
        while (!this->ready_to_data_) {
          this->data_barier_.wait(lock);
        }

        this->ready_to_data_ = false;
        return this->data_.write_value_async(data).then(
            [pthis = this->shared_from_this()]() {
              auto this_ = static_cast<_async_buffer *>(pthis.get());
              this_->data_mutex_.lock();
              this_->ready_to_data_ = true;
              this_->data_barier_.notify_one();
              this_->data_mutex_.unlock();
            });
      }

      vds::async_task<void> write_async(const item_type *data, size_t data_size) override {
        std::unique_lock<std::mutex> lock(this->data_mutex_);
        while (!this->ready_to_data_) {
          this->data_barier_.wait_for(lock, std::chrono::microseconds(100));
        }

        this->ready_to_data_ = false;

        return this->data_.write_async(data, data_size).then(
            [pthis = this->shared_from_this()]() {
              auto this_ = static_cast<_async_buffer *>(pthis.get());
              this_->data_mutex_.lock();
              this_->ready_to_data_ = true;
              this_->data_barier_.notify_one();
              this_->data_mutex_.unlock();
            });
      }

      vds::async_task<size_t /*readed*/> read_async(item_type *buffer, size_t buffer_size) {
        return this->data_.read_async(buffer, buffer_size);
      }

    private:
      bool ready_to_data_;
      std::condition_variable data_barier_;
      std::mutex data_mutex_;
      continuous_buffer<item_type> data_;
    };
  };
  /////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <typename item_type, typename source_type, typename target_type>
  class _continue_copy_stream_async : public std::enable_shared_from_this<_continue_copy_stream_async<item_type, source_type, target_type>>
  {
  public:
    _continue_copy_stream_async(
      const service_provider & sp,
      const std::shared_ptr<source_type> & source,
      const std::shared_ptr<target_type> & target)
    : sp_(sp), source_(source), target_(target)
    {
    }
    
    static vds::async_task<void> _continue_copy(
      const std::shared_ptr<_continue_copy_stream_async> & context)
    {
      return context->source_->read_async(
        context->buffer_,
        sizeof(context->buffer_) / sizeof(context->buffer_[0])).then(
        [context](size_t readed){
          return context->target_->write_async(context->buffer_, readed).
            then([context, is_eof = (0 == readed)](){
              if(is_eof){
                return vds::async_task<void>::empty();
              }
              else {
                return _continue_copy(context);
              }              
            });
        }
      );    
    }
        
  private:
    service_provider sp_;
    std::shared_ptr<source_type> source_;
    std::shared_ptr<target_type> target_;
    item_type buffer_[1024];
  };
  
  
  template <typename item_type>
  inline vds::async_task<void> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<item_type>> & source,
    const std::shared_ptr<async_buffer<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream_async<item_type, continuous_buffer<item_type>, async_buffer<item_type>>>(sp, source, target);
    return _continue_copy_stream_async<item_type, continuous_buffer<item_type>, async_buffer<item_type>>::_continue_copy(context);
  }
  
  template <typename item_type>
  inline vds::async_task<void> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<async_buffer<item_type>> & source,
    const std::shared_ptr<continuous_buffer<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream_async<item_type, async_buffer<item_type>, continuous_buffer<item_type>>>(sp, source, target);
    return _continue_copy_stream_async<item_type, async_buffer<item_type>, continuous_buffer<item_type>>::_continue_copy(context);
  }
  
  template <typename item_type>
  inline vds::async_task<void> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<item_type>> & source,
    const std::shared_ptr<continuous_buffer<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream_async<item_type, continuous_buffer<item_type>, continuous_buffer<item_type>>>(sp, source, target);
    return _continue_copy_stream_async<item_type, continuous_buffer<item_type>, continuous_buffer<item_type>>::_continue_copy(context);
  }
  
  template <typename item_type>
  inline vds::async_task<void> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<item_type>> & source,
    const std::shared_ptr<stream_async<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream_async<item_type, continuous_buffer<item_type>, stream_async<item_type>>>(sp, source, target);
    return _continue_copy_stream_async<item_type, continuous_buffer<item_type>, stream_async<item_type>>::_continue_copy(context);
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <typename item_type, typename source_type, typename target_type>
  class _continue_copy_stream : public std::enable_shared_from_this<_continue_copy_stream<item_type, source_type, target_type>>
  {
  public:
    _continue_copy_stream(
      const service_provider & sp,
      const std::shared_ptr<source_type> & source,
      const std::shared_ptr<target_type> & target)
      : sp_(sp), source_(source), target_(target)
    {
    }

    static vds::async_task<void> _continue_copy(
      const std::shared_ptr<_continue_copy_stream> & context)
    {
      return context->source_->read_async(
        context->buffer_,
        sizeof(context->buffer_) / sizeof(context->buffer_[0])).then(
          [context](size_t readed) {
        context->target_->write(context->buffer_, readed);
        if (0 == readed) {
          return vds::async_task<void>::empty();
        }
        else {
          return _continue_copy(context);
        }
      });
    }

  private:
    service_provider sp_;
    std::shared_ptr<source_type> source_;
    std::shared_ptr<target_type> target_;
    item_type buffer_[1024];
  };

  template <typename item_type>
  inline vds::async_task<void> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<item_type>> & source,
    const std::shared_ptr<stream<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream<item_type, continuous_buffer<item_type>, stream<item_type>>>(sp, source, target);
    return _continue_copy_stream<item_type, continuous_buffer<item_type>, stream<item_type>>::_continue_copy(context);
  }

  template <typename item_type>
  inline vds::async_task<void> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<async_buffer<item_type>> & source,
    const std::shared_ptr<stream<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream<item_type, async_buffer<item_type>, stream<item_type>>>(sp, source, target);
    return _continue_copy_stream<item_type, async_buffer<item_type>, stream<item_type>>::_continue_copy(context);
  }

}

#endif // __VDS_CORE_ASYNC_STREAM_H_
 
